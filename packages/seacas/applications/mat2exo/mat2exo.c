/**********************************************************************/
/* matlab mat file to exodus II.  This takes .mat files exactly as
   generated by the tool exo2mat written by mrtabba and converts them
   back to exodus II format

   rmnaeth. August 8, 2003

   modified by D. Todd Griffith on 12/09/2005
   * modifcations include:
   1) writes global, nodal and element variable names
   2) writes global, nodal and elemnent variable results
   3) writes complete set of time steps (previous version
          skipped first step) 
   4) writes complete node set information (node set numbers, 
          dist. factors, etc)
   5) writes complete side set information (side set numbers, 
          dist. factors, etc)

   modified by D. Todd Griffith on 12/16/2005
   * side set distribution factors now written as double (not int)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <assert.h>
#include <exodusII.h>
#include "mat.h"
#include "add_to_log.h"

/* The maximum buffer size we will need is MAX_LINE_LENGTH (defined in
   exodusII.h to be 80) x the number of informational records. */
#define MAX_INFO_RECORDS 20

/**********************************************************************/
MATFile *mat_file=0;  /* file for binary .mat input */

/**********************************************************************/
static char *qainfo[] =
{
  "mat2exo",
  "$Date: 2009/03/17 16:54:56 $",
  "$Revision: 1.6 $",
};

/**********************************************************************/
int matGetStr (char *name,char *str);
int matGetDbl (char *name,int n1,int n2, double *pd);
int matGetInt (char *name,int n1,int n2, int *pd);
int matArrNRow (char *name);
int matArrNCol (char *name);
int *d2i (double *dscr, int n);
void del_arg(int *argc, char* argv[], int j);

/**********************************************************************/
int main (int argc, char *argv[]){

  char  
    *str,**str2,*line,*curr;
    
  const char* ext=".exo";

  int   
    i,j,k,n,n1,cpu_word_size,io_word_size,exo_file,err,
    num_axes,num_nodes,num_elements,num_blocks,
    num_side_sets,num_node_sets,num_time_steps,
    num_qa_lines,num_info_lines,num_global_vars,
    num_nodal_vars,num_element_vars,*ids,*iscr,
    *nsssides,*nssdfac,*elem_list,*side_list,
    *nnsnodes,*nnsdfac,*node_list;

  double
    f,*scr,*x,*y,*z,
    *ascr,*bscr,*cscr,*dscr,*escr;

  char * blknames = NULL;
  double *num_elem_in_block = NULL;

  str = (char *) calloc((MAX_LINE_LENGTH*MAX_INFO_RECORDS),sizeof(char));
  
  /* QA Info */
  printf("%s: %s, %s\n", qainfo[0], qainfo[2], qainfo[1]);

  /* usage message*/
  if(argc != 2){
    printf("%s matlab_file_name.\n",argv[0]);
    printf("   the matlab_file_name is required\n");
    printf("%d", argc);
    exit(1);
  }
  
  /*open input file*/
  mat_file = matOpen(argv[1], "r");
  if (mat_file == NULL) {
    printf("Error opening file %s\n", argv[1]);
    return(1);
  }

  /*open output file*/
  cpu_word_size=sizeof(double);
  io_word_size=0;
  /* QA records */
  ext=".exo";
  line = (char *) calloc ((MAX_LINE_LENGTH+1),sizeof(char *));
  strcpy(line,argv[1]);
  strtok(line,".");  /* QA records */
  strcat(line,ext);
  exo_file = ex_create(line,EX_CLOBBER,&cpu_word_size,&io_word_size);
  if (exo_file < 0){
    printf("error creating %s\n",line);
    exit(1);
  }

  /* print */
  fprintf(stderr,"translating %s to %s ... ",argv[1],line);
  
  /* read database parameters */
  matGetDbl("naxes", 1, 1,&f);num_axes=(int)f;
  matGetDbl("nnodes", 1, 1,&f);num_nodes=(int)f;
  matGetDbl("nelems", 1, 1,&f);num_elements=(int)f;
  matGetDbl("nblks",  1, 1,&f);num_blocks=(int)f;
  matGetDbl("nnsets", 1, 1,&f);num_node_sets=(int)f;
  matGetDbl("nssets", 1, 1,&f);num_side_sets=(int)f;
  matGetDbl("nsteps", 1, 1,&f);num_time_steps=(int)f;
  matGetDbl("ngvars", 1, 1,&f);num_global_vars=(int)f;
  matGetDbl("nnvars", 1, 1,&f);num_nodal_vars=(int)f;
  matGetDbl("nevars", 1, 1,&f);num_element_vars=(int)f;

  /*export parameters */
  err = ex_put_init(exo_file,line,
		    num_axes,num_nodes,num_elements,num_blocks,
		    num_node_sets,num_side_sets);
  free(line);
  
  if ( num_global_vars > 0 ){
    err=ex_put_var_param(exo_file,"g",num_global_vars);
  }
  
  if ( num_nodal_vars > 0 ){
    err=ex_put_var_param(exo_file,"n",num_nodal_vars);
  }
  
  if ( num_element_vars > 0 ){
    err=ex_put_var_param(exo_file,"e",num_element_vars);
  }

  /* QA records */
  /* exo2mat does not seem to export QA records? */
  num_qa_lines = 0;

  /* information records */
#if 0
  if ( !matGetStr("info",str) ) {
    num_info_lines = 0;
    curr = str;
    curr = strtok(curr,"\n");
    while(curr!=NULL){
      num_info_lines++;
      curr=strtok(NULL,"\n");
    }
    str2 = (char **) calloc(num_info_lines,sizeof(char *));
    /* We have to refetch the string because strtok breaks the original copy */
    matGetStr("info",str);
    curr = str;
    curr = strtok(curr,"\n");
    for (i=0;i<num_info_lines;i++){
      str2[i]=curr;
      curr=strtok(NULL,"\n");
    }
    err = ex_put_info(exo_file,num_info_lines,str2);
    free(str2);
  }
#endif

  /* nodal coordinates */
  x = (double *) calloc(num_nodes,sizeof(double));
  y = (double *) calloc(num_nodes,sizeof(double));
  if (num_axes == 3) 
    z = (double *) calloc(num_nodes,sizeof(double));
  else 
    z = NULL;
  matGetDbl("x0", num_nodes, 1, x);
  matGetDbl("y0", num_nodes, 1, y);
  if (num_axes == 3)
    matGetDbl("z0", num_nodes,1,z);
  err = ex_put_coord(exo_file,x,y,z);
  free(x);
  free(y);
  if (num_axes == 3){ 

    free(z);
  }
  

  /* side sets (section by dgriffi) */
  if(num_side_sets > 0){ 
     
    /* ssids */
    scr = (double *) calloc(num_side_sets,sizeof(double)); 
    matGetDbl("ssids",num_side_sets, 1,scr);
    ids=d2i(scr,num_side_sets);
    /* nsssides */
    ascr = (double *) calloc(num_side_sets,sizeof(double));
    matGetDbl("nsssides",num_side_sets,1,ascr);
    nsssides=d2i(ascr,num_side_sets);
    /* nssdfac */
    bscr = (double *) calloc(num_side_sets,sizeof(double));
    matGetDbl("nssdfac",num_side_sets,1,bscr);
    nssdfac=d2i(bscr,num_side_sets);

    for(i=0;i<num_side_sets;i++){
      err = ex_put_side_set_param(exo_file,ids[i],nsssides[i],nssdfac[i]);
      cscr = (double *) calloc(nsssides[i],sizeof(double));
      dscr = (double *) calloc(nsssides[i],sizeof(double));
      escr = (double *) calloc(nssdfac[i],sizeof(double));
           
      sprintf(str,"sselem%02d",i+1);
      matGetDbl(str,nsssides[i],1,cscr);
      elem_list=d2i(cscr,nsssides[i]);
      free(cscr);
      sprintf(str,"ssside%02d",i+1);
      matGetDbl(str,nsssides[i],1,dscr);
      side_list=d2i(dscr,nsssides[i]);
      free(dscr);
      err = ex_put_side_set(exo_file,ids[i],elem_list,side_list);
      free(elem_list);
      free(side_list);
      sprintf(str,"ssfac%02d",i+1);
      matGetDbl(str,nssdfac[i],1,escr);
      err = ex_put_side_set_dist_fact(exo_file,ids[i],escr);
      free(escr);      
    }
   
    free(nsssides);
    free(nssdfac);
    free(ids);
    free(scr);
    free(ascr);
    free(bscr);
   
  }  

  /* node sets (section by dgriffi) */
  if(num_node_sets > 0){ 
     
    /* nsids */
    scr = (double *) calloc(num_node_sets,sizeof(double)); 
    matGetDbl("nsids",num_node_sets, 1,scr);
    ids=d2i(scr,num_node_sets);
    /* nnsnodes */
    ascr = (double *) calloc(num_node_sets,sizeof(double));
    matGetDbl("nnsnodes",num_node_sets,1,ascr);
    nnsnodes=d2i(ascr,num_node_sets);
    /* nnsdfac */
    bscr = (double *) calloc(num_node_sets,sizeof(double));
    matGetDbl("nnsdfac",num_node_sets,1,bscr);
    nnsdfac=d2i(bscr,num_node_sets);

    for(i=0;i<num_node_sets;i++){
      err = ex_put_node_set_param(exo_file,ids[i],nnsnodes[i],nnsdfac[i]);
      dscr = (double *) calloc(nnsnodes[i],sizeof(double));
      escr = (double *) calloc(nnsdfac[i],sizeof(double));
           
      sprintf(str,"nsnod%02d",i+1);
      matGetDbl(str,nnsnodes[i],1,dscr);
      node_list=d2i(dscr,nnsnodes[i]);
      free(dscr);
      err = ex_put_node_set(exo_file,ids[i],node_list);
      free(node_list);
      
      sprintf(str,"nsfac%02d",i+1);
      matGetDbl(str,nnsdfac[i],1,escr);
      err = ex_put_node_set_dist_fact(exo_file,ids[i],escr);
      free(escr);      
    }
   
    free(nnsdfac);
    free(nnsnodes);
    free(ids);
    free(scr);
    free(ascr);
    free(bscr);
   
  }  


  /* element blocks */ 
  /* get elem block ids */
  scr = (double *) calloc(num_blocks,sizeof(double));
  matGetDbl("blkids",num_blocks,1,scr);
  ids= d2i(scr,num_blocks);
  free(scr);

  /* get elem block types */
  blknames = (char *) calloc(num_blocks*MAX_STR_LENGTH,sizeof(char));
  matGetStr("blknames",blknames);
  num_elem_in_block = (double *) calloc(num_blocks,sizeof(double));
  curr = blknames;
  curr = strtok(curr,"\n");
  for(i=0;i<num_blocks;i++){
    sprintf(str,"blk%02d",i+1);
    n1 = matArrNRow(str);
    n = matArrNCol(str);
    scr = (double *) calloc(n*n1,sizeof(double));
    matGetDbl(str,n1,n,scr);
    num_elem_in_block[i]=n;
    iscr=d2i(scr,n*n1);
    err = ex_put_elem_block(exo_file,ids[i],curr,n,n1,0);
    err = ex_put_elem_conn(exo_file,ids[i],iscr);
    free(iscr);
    free(scr);
    curr = strtok(NULL, "\n");
  }
  free(blknames);

  /* time values */
  if (num_time_steps > 0 ) {
    scr = (double *) calloc(num_time_steps,sizeof(double));
    matGetDbl( "time", num_time_steps, 1,scr);
    for (i=0;i<num_time_steps;i++){
      err=ex_put_time(exo_file,i+1,&scr[i]);
    }
    free(scr); 
  }
  
  /* global variables */
  if (num_global_vars > 0 ){
    matGetStr("gnames",str);
    str2 = (char **) calloc(num_global_vars,sizeof(char*));
    curr = strtok(str,"\n");
    for(i=0;i<num_global_vars;i++){
      str2[i]=curr;
      curr = strtok(NULL,"\n");
    }
    err = ex_put_var_names(exo_file, "g", num_global_vars, str2);
    free(str2);

    {
      double * global_var_vals;
      double * temp;
      global_var_vals = (double *) calloc(num_global_vars,sizeof(double));
      temp            = (double *) calloc(num_time_steps,sizeof(double));
      for (i=0;i<num_time_steps;i++) {
	
	for (j=0;j<num_global_vars;j++) {
	  sprintf(str,"gvar%02d",j+1);
	  matGetDbl(str,num_time_steps,1,temp);
	  global_var_vals[j]=temp[i];}
	
	err = ex_put_glob_vars(exo_file,i+1,num_global_vars,global_var_vals);
	
      }
      free(temp);
      free(global_var_vals);
    }
  }

  
  /* nodal variables */ /* section by dtg */

  if (num_nodal_vars > 0){
    matGetStr("nnames",str);
    str2 = (char **) calloc(num_nodal_vars,sizeof(char*));
    curr = strtok(str,"\n");
    for(i=0;i<num_nodal_vars;i++){
      str2[i]=curr;
      curr = strtok(NULL,"\n");
    }
    err = ex_put_var_names(exo_file, "n", num_nodal_vars, str2);	
    free(str2);
    {
      double * nodal_var_vals;
      for (i=0;i<num_nodal_vars;i++) {
	nodal_var_vals = (double *) calloc(num_nodes*num_time_steps,sizeof(double));
	sprintf(str,"nvar%02d",i+1);
	matGetDbl(str,num_nodes,num_time_steps,nodal_var_vals);
	for (j=0;j<num_time_steps;j++) {
	  err = ex_put_nodal_var(exo_file,j+1,i+1,num_nodes,nodal_var_vals+num_nodes*j);
	}
	free(nodal_var_vals); 
      }
    }
  }

  /* elemental variables */ /* section by dtg */
  
  if (num_element_vars > 0){
    matGetStr("enames",str);
    str2 = (char **) calloc(num_element_vars,sizeof(char*));
    curr = strtok(str,"\n");
    for(i=0;i<num_element_vars;i++){
      str2[i]=curr;
      curr = strtok(NULL,"\n");
    }
    err = ex_put_var_names(exo_file, "e", num_element_vars, str2);	
    free(str2);
    {
      double * element_var_vals;
      int * temp2;
      temp2 = d2i(num_elem_in_block,num_blocks);
     
      for (i=0;i<num_element_vars;i++) {
	element_var_vals = (double *) calloc(num_elements*num_time_steps,sizeof(double));       
	sprintf(str,"evar%02d",i+1);
	matGetDbl(str,num_elements,num_time_steps,element_var_vals);
	n=0;       
	for (j=0;j<num_time_steps;j++) {
	  for (k=0;k<num_blocks;k++) {
	    err = ex_put_elem_var(exo_file,j+1,i+1,ids[k],temp2[k],element_var_vals+n);
	    n=n+temp2[k];
	  }
	}
	free(element_var_vals);
      }
      free(temp2);
    }
  }
  free(ids); /* Allocated by d2i in element block section above */

  /* node and element number maps */
  scr = (double *) calloc (num_nodes,sizeof(double));
  if ( !matGetDbl("node_num_map",num_nodes,1,scr)){
    ids = d2i(scr,num_nodes);
    err = ex_put_node_num_map(exo_file,ids);
    free(ids);
  }
  free(scr);

  scr = (double *) calloc (num_elements,sizeof(double));
  if ( !matGetDbl("elem_num_map",num_elements,1,scr)){
    ids = d2i(scr,num_elements);
    err = ex_put_elem_num_map(exo_file,ids);
    free(ids);
  }
  free(scr);
  free(num_elem_in_block);
  
  /* close exo file */
  ex_close(exo_file);
  
  /* close mat file */
  matClose(mat_file);

  /* */
  fprintf(stderr,"done.\n");

  free(str);

  /* exit status */
  add_to_log("mat2exo", 0);
  return(0);
}

/**********************************************************************/
int matGetStr (char *name,char *str)
{
  int strlen;
  /* this is for Matlab 6.5 */
    mxArray *pa;
    if ( !(pa = matGetVariable(mat_file,name)) )
	return -1;
    strlen = mxGetN(pa);
    if(mxGetM(pa)!=1)
      printf("Error: Multiline string copy attempted\n");
    mxGetString(pa,str,strlen);
    mxDestroyArray(pa);
    return 0;
}

/**********************************************************************/
int matGetDbl (char *name,int n1,int n2, double *pd)
{
    mxArray *pa;
    if ( !(pa=matGetVariable(mat_file,name)) )
	return -1;
    memcpy(pd,mxGetPr(pa),n1*n2*sizeof(double));
    mxDestroyArray(pa);
    return 0;
}

/**********************************************************************/
int matGetInt (char *name,int n1,int n2, int *pd)
{
    mxArray *pa;
    if ( !(pa=matGetVariable(mat_file,name)) )
	return -1;
    memcpy(pd,mxGetPr(pa),n1*n2*sizeof(int));
    mxDestroyArray(pa);
    return 0;
}

/**********************************************************************/
int matArrNRow (char *name)
{
    mxArray *pa;
    int len;
    if ( !(pa=matGetVariable(mat_file,name)) )
	return -1;
    len = mxGetM(pa);
    mxDestroyArray(pa);
    return len;
}

/**********************************************************************/
int matArrNCol (char *name)
{
    mxArray *pa;
    int len;
    if ( !(pa=matGetVariable(mat_file,name)) )
	return -1;
    len = mxGetN(pa);
    mxDestroyArray(pa);
    return len;
}


/**********************************************************************/   
/* double-to-int copy (since MATLAB needs double)*/
int *d2i (double *dscr, int n)
{
  int i;
  int *scr;
  scr = (int *) calloc(n,sizeof(int));
  for (i=0;i<n;i++)
    scr[i]=(int)dscr[i];
  return scr;
}

/**********************************************************************/
/* remove an argument from the list */
void del_arg(int *argc, char* argv[], int j)
{
  int jj;
  for (jj=j+1;jj<*argc;jj++)
    argv[jj-1]=argv[jj];
  (*argc)--;
  argv[*argc]=0;
}
