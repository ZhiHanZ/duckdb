#include "duckdb/parser/expression/subquery_expression.hpp"
#include "duckdb/planner/binder.hpp"
#include "duckdb/planner/expression/bound_cast_expression.hpp"
#include "duckdb/planner/expression/bound_subquery_expression.hpp"
#include "duckdb/planner/expression_binder.hpp"

namespace duckdb {

class BoundSubqueryNode : public QueryNode {
public:
	BoundSubqueryNode(shared_ptr<Binder> subquery_binder, unique_ptr<BoundQueryNode> bound_node,
	                  unique_ptr<SelectStatement> subquery)
	    : QueryNode(QueryNodeType::BOUND_SUBQUERY_NODE), subquery_binder(move(subquery_binder)),
	      bound_node(move(bound_node)), subquery(move(subquery)) {
	}

	shared_ptr<Binder> subquery_binder;
	unique_ptr<BoundQueryNode> bound_node;
	unique_ptr<SelectStatement> subquery;

	const vector<unique_ptr<ParsedExpression>> &GetSelectList() const override {
		throw Exception("Cannot get select list of bound subquery node");
	}

	unique_ptr<QueryNode> Copy() override {
		throw Exception("Cannot copy bound subquery node");
	}
};

BindResult ExpressionBinder::BindExpression(SubqueryExpression &expr, idx_t depth) {
	if (expr.subquery->node->type != QueryNodeType::BOUND_SUBQUERY_NODE) {
		D_ASSERT(depth == 0);
		// first bind the actual subquery in a new binder
		auto subquery_binder = Binder::CreateBinder(context, &binder);
		subquery_binder->can_contain_nulls = true;
		auto bound_node = subquery_binder->BindNode(*expr.subquery->node);
		// check the correlated columns of the subquery for correlated columns with depth > 1
		for (idx_t i = 0; i < subquery_binder->correlated_columns.size(); i++) {
			CorrelatedColumnInfo corr = subquery_binder->correlated_columns[i];
			if (corr.depth > 1) {
				// depth > 1, the column references the query ABOVE the current one
				// add to the set of correlated columns for THIS query
				corr.depth -= 1;
				binder.AddCorrelatedColumn(corr);
			}
		}
		if (expr.subquery_type != SubqueryType::EXISTS && bound_node->types.size() > 1) {
			throw BinderException("Subquery returns %zu columns - expected 1", bound_node->types.size());
		}
		auto prior_subquery = move(expr.subquery);
		expr.subquery = make_unique<SelectStatement>();
		expr.subquery->node =
		    make_unique<BoundSubqueryNode>(move(subquery_binder), move(bound_node), move(prior_subquery));
	}
	// now bind the child node of the subquery
	if (expr.child) {
		// first bind the children of the subquery, if any
		string error = Bind(&expr.child, depth);
		if (!error.empty()) {
			return BindResult(error);
		}
	}
	// both binding the child and binding the subquery was successful
	D_ASSERT(expr.subquery->node->type == QueryNodeType::BOUND_SUBQUERY_NODE);
	auto bound_subquery = (BoundSubqueryNode *)expr.subquery->node.get();
	auto child = (BoundExpression *)expr.child.get();
	auto subquery_binder = move(bound_subquery->subquery_binder);
	auto bound_node = move(bound_subquery->bound_node);
	LogicalType return_type =
	    expr.subquery_type == SubqueryType::SCALAR ? bound_node->types[0] : LogicalType(LogicalTypeId::BOOLEAN);
	D_ASSERT(return_type.id() != LogicalTypeId::UNKNOWN);

	auto result = make_unique<BoundSubqueryExpression>(return_type);
	if (expr.subquery_type == SubqueryType::ANY) {
		// ANY comparison
		// cast child and subquery child to equivalent types
		D_ASSERT(bound_node->types.size() == 1);
		auto compare_type = LogicalType::MaxLogicalType(child->expr->return_type, bound_node->types[0]);
		child->expr = BoundCastExpression::AddCastToType(move(child->expr), compare_type);
		result->child_type = bound_node->types[0];
		result->child_target = compare_type;
	}
	result->binder = move(subquery_binder);
	result->subquery = move(bound_node);
	result->subquery_type = expr.subquery_type;
	result->child = child ? move(child->expr) : nullptr;
	result->comparison_type = expr.comparison_type;

	return BindResult(move(result));
}

} // namespace duckdb
