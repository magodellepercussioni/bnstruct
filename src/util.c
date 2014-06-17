#include "util.h"

SEXP score_node( SEXP data, SEXP node_sizes, SEXP ni, SEXP pars, SEXP ess )
{
  SEXP score;
  PROTECT( score = allocVector(REALSXP, 1) );
  *REAL(score) = log_lik( INTEGER(data), ncols(data), nrows(data), INTEGER(node_sizes),
    *INTEGER(ni), INTEGER(pars), length(pars), *REAL(ess) );
  
  UNPROTECT(1);
  return score;
}

SEXP is_acyclic( SEXP graph )
{
  int n_nodes = nrows(graph);
  int * g = INTEGER(graph);
  int rem[n_nodes];
  int leaves[n_nodes];
  int rem_count = 0;
  int i,j,flag,aleaf;
  
  // allocate and protect output
  SEXP test;
  PROTECT( test = allocVector(INTSXP, 1) );
  
  for( i = 0; i < n_nodes; i++ )
  {
    rem[i] = 0;
    leaves[i] = 0;
  }
  
  int gtemp[n_nodes * n_nodes]; 
  for (i = 0; i < n_nodes * n_nodes; i++)
    gtemp[i] = g[i];
  
  while( rem_count < n_nodes ) // still some edges to remove
  {
    aleaf = 0;
    // find leaves among non-removed nodes
    for( i = 0; i < n_nodes; i++ )
      if( !rem[i] )
      {
        flag = gtemp[i];
        for( j = 1; j < n_nodes; j++ )
          flag |= gtemp[j*n_nodes + i];
        if( !flag ) // a leaf
        {
          // printf("Leaf: %d\n",i);
          aleaf = 1;
          leaves[i] = 1;
          rem[i] = 1;
          rem_count++;
        }
      }
    
    // test for leaves
    if( !aleaf )
    {
      *INTEGER(test) = 0;
      UNPROTECT(1);
      return test ;
    }
    else // remove edges incident on leaves
    {
      for(j = 0; j < n_nodes; j++)
        if( leaves[j] )
        {
          leaves[j] = 0; // reset for next iteration
          for( i = 0; i < n_nodes; i++ )
            gtemp[j * n_nodes + i] = 0;
        }
    }
    // printf("Next\n");
  }
  
  // acyclic
  *INTEGER(test) = 1;
  UNPROTECT(1);
  return test ;
}

double log_lik( unsigned int * d, unsigned int n_nodes, unsigned int n_cases, unsigned int * ns, 
	unsigned int ni, unsigned int * pa, unsigned int n_pa, double alpha )
{
	int i, j, index, elmt, stride;
	double acc, ll;
	
	// utility vectors
	int cum_prod_sizes[n_pa+2]; 
	cum_prod_sizes[0] = 1;
	cum_prod_sizes[1] = ns[ni];
	for( i = 0; i < n_pa; i++ )
		cum_prod_sizes[i+2] = cum_prod_sizes[i+1] * ns[pa[i]];
	
	int strides[n_pa + 1];
	strides[0] = ni * n_cases;
	for( i = 0; i < n_pa; i++ )
		strides[i+1] = pa[i] * n_cases;
	
	// utility variables
	int prod_sizes = cum_prod_sizes[n_pa+1];
	int prod_sizes_pa = prod_sizes / ns[ni];
	// int n_na = 0;
	double prior = alpha / prod_sizes;
	double alpha_ij = alpha / prod_sizes_pa;
	
	// vector holding counts (LARGE)
	double * counts = calloc( prod_sizes, sizeof(double) ); 
	
	// compute counts, skipping NAs
	for( i = 0; i < n_cases; i++ )
	{
		index = 0;
		// sum using strides
		for( j = 0; j < n_pa + 1; j++ )
		{
			elmt = d[ i + strides[j] ];
			if( elmt == NA_INTEGER )
			//{
			//	n_na++;
				break;
			//}
			index += (elmt - 1) * cum_prod_sizes[j];
		}
		// check if NA encountered
		if( j < n_pa + 1 )
			continue;
    
		counts[index] += 1;			
	}
	
	// correct for NAs
	//for( i = 0; i < prod_sizes; i++ )
	//	counts[i] += n_na * (counts[i] + prior) / ( n_cases - n_na + alpha );
		
	// compute log likelihood
	ll = prod_sizes_pa*lgamma(alpha_ij) - prod_sizes*lgamma(prior);
	for( i = 0; i < prod_sizes; i++ )
		ll += lgamma(prior + counts[i]);
	
	// accumulate parent counts, by summing over ni
	for( i = 0; i < prod_sizes_pa; i++ )
	{
		stride = i * ns[ni];
		acc = counts[stride];
		for( j = 1; j < ns[ni]; j++ )
			acc += counts[stride + j];
		ll -= lgamma(alpha_ij + acc);
	}
  
  free( counts );
	
	return ll;
}

SEXP compute_counts_nas( SEXP data, SEXP node_sizes )
{
	int i,j,index,elmt,stride;
	// inputs
	int * d = INTEGER(data);
	int n_nodes = ncols(data);
	int n_cases = nrows(data);
	int * ns = INTEGER(node_sizes);
	
	// internal structure
	int cum_prod_sizes[n_nodes+1]; 
	cum_prod_sizes[0] = 1;
	for( i = 1; i < n_nodes + 1; i++ )
		cum_prod_sizes[i] = cum_prod_sizes[i-1] * ns[i-1];
	
	// precomputed here for speed
	int strides[n_nodes];
	for( i = 0; i < n_nodes; i++ )
		strides[i] = i * n_cases;
	
	// allocate output
	SEXP result;
	PROTECT( result = allocVector(REALSXP, cum_prod_sizes[n_nodes]) );
	double * counts = REAL(result);
	memset( counts, 0, sizeof(double) * cum_prod_sizes[n_nodes] );
	setAttrib(result, R_DimSymbol, node_sizes);
	
	for( i = 0; i < n_cases; i++ )
	{
		index = 0;
		// sum using strides
		for( j = 0; j < n_nodes; j++ )
		{
			elmt = d[ i + strides[j] ];
			if( elmt == NA_INTEGER )
				break;
			index += (elmt - 1) * cum_prod_sizes[j];
		}
		// check if NA encountered
		if( j < n_nodes )
			continue;
    
		counts[index] += 1;			
	}
	
	UNPROTECT(1);
	return result;
}

SEXP compute_counts( SEXP data, SEXP node_sizes )
{
  int i,j,index,elmt;
	// inputs
	int * d = INTEGER(data);
	int n_nodes = ncols(data);
	int n_cases = nrows(data);
	int * ns = INTEGER(node_sizes);
	
	// internal structure
	int cum_prod_sizes[n_nodes+1]; 
	cum_prod_sizes[0] = 1;
	for( i = 1; i < n_nodes + 1; i++ )
		cum_prod_sizes[i] = cum_prod_sizes[i-1] * ns[i-1];
	
	// allocate output
	SEXP result;
	PROTECT( result = allocVector(REALSXP, cum_prod_sizes[n_nodes]) );
	double * counts = REAL(result);
	memset( counts, 0, sizeof(double) * cum_prod_sizes[n_nodes] );
	setAttrib(result, R_DimSymbol, node_sizes);
	
	// compute counts
	for( i = 0; i < n_cases; i++ )
	{
		index = 0;
		for( j = 0; j < n_nodes; j++ )
			index += (d[ i + j*n_cases ] - 1) * cum_prod_sizes[j];
		counts[index] += 1;			
	}
	
	UNPROTECT(1);
	return result;
}