#' Register a data frame as a virtual table
#'
#' `duckdb_register()` registers a data frame as a virtual table (view)
#'  in a DuckDB connection.
#'  No data is copied.
#'
#' `duckdb_unregister()` unregisters a previously registered data frame.
#' @param conn A DuckDB connection, created by `dbConnect()`.
#' @param name The name for the virtual table that is registered or unregistered
#' @param df A `data.frame` with the data for the virtual table
#' @return These functions are called for their side effect.
#' @export
#' @examples
#' con <- dbConnect(duckdb())
#'
#' data <- data.frame(a = 1:3, b = letters[1:3])
#'
#' duckdb_register(con, "data", data)
#' dbReadTable(con, "data")
#'
#' duckdb_unregister(con, "data")
#' try(dbReadTable(con, "data"))
#'
#' dbDisconnect(con)
duckdb_register <- function(conn, name, df) {
  stopifnot(dbIsValid(conn))
  .Call(duckdb_register_R, conn@conn_ref, as.character(name), as.data.frame(df))
  invisible(TRUE)
}

#' @rdname duckdb_register
#' @export
duckdb_unregister <- function(conn, name) {
  stopifnot(dbIsValid(conn))
  .Call(duckdb_unregister_R, conn@conn_ref, as.character(name))
  invisible(TRUE)
}

#' Register an Arrow data source as a virtual table
#'
#' `duckdb_register_arrow()` registers an Arrow data source as a virtual table (view)
#'  in a DuckDB connection.
#'  No data is copied.
#'
#' `duckdb_unregister_arrow()` unregisters a previously registered data frame.
#' @param conn A DuckDB connection, created by `dbConnect()`.
#' @param name The name for the virtual table that is registered or unregistered
#' @param arrow_scannable A scannable Arrow-object
#' @return These functions are called for their side effect.
#' @export
duckdb_register_arrow <- function(conn, name, arrow_scannable) {
  stopifnot(dbIsValid(conn))

    # create some R functions to pass to c-land
    export_fun <- function(arrow_scannable, stream_ptr, projection=NULL, filter=TRUE) {
        arrow::Scanner$create(arrow_scannable, projection, filter)$ToRecordBatchReader()$export_to_c(stream_ptr)
    }
   # pass some functions to c land so we don't have to look them up there
   function_list <- list(export_fun, arrow::Expression$create, arrow::Expression$field_ref, arrow::Expression$scalar)
  .Call(duckdb_register_arrow_R, conn@conn_ref, as.character(name), function_list, arrow_scannable)
  invisible(TRUE)
}

#' @rdname duckdb_register_arrow
#' @export
duckdb_unregister_arrow <- function(conn, name) {
  stopifnot(dbIsValid(conn))
  .Call(duckdb_unregister_arrow_R, conn@conn_ref, as.character(name))
  invisible(TRUE)
}