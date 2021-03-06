% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/AllGenerics.R, R/BNDataset-methods.R
\name{has.imputed.data}
\alias{has.imputed.data}
\alias{has.imputed.data,BNDataset-method}
\alias{has.imputed.data,BNDataset}
\title{check if a BNDataset contains impited data.}
\usage{
has.imputed.data(x)

\S4method{has.imputed.data}{BNDataset}(x)
}
\arguments{
\item{x}{a \code{\link{BNDataset}}.}
}
\description{
Check whether a \code{\link{BNDataset}} object actually contains imputed data.
}
\examples{
\dontrun{
x <- BNDataset()
has.imputed.data(x) # FALSE

x <- read.dataset(x, "file.header", "file.data")
has.imputed.data(x) # FALSE, since read.dataset() actually reads raw data.

x <- impute(x)
has.imputed.data(x) # TRUE
}

}
\seealso{
\code{\link{has.raw.data}}, \code{\link{raw.data}}, \code{\link{imputed.data}}
}
