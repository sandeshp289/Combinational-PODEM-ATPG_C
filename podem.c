///////////////////////////////////////////////////////////////////////////////////
// File       : podem.c
// Language   : C/C++
// Description: PODEM ATPG for Combinational Circuits 
// Author     : Sandesh Prabhakar
// Created    : November 1, 2021
// Modified   : August 15, 2026
// Version    : 1.0
//
// Copyright (c) 2026 Sandesh Prabhakar
// All rights reserved
//////////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum
{
   G_JUNK,         /* 0 */
   G_INPUT,        /* 1 */
   G_OUTPUT,       /* 2 */
   G_XOR,          /* 3 */
   G_XNOR,         /* 4 */
   G_DFF,          /* 5 */
   G_AND,          /* 6 */
   G_NAND,         /* 7 */
   G_OR,           /* 8 */
   G_NOR,          /* 9 */
   G_NOT,          /* 10 */
   G_BUF,          /* 11 */
   G_TIE0,         /* 12 */
   G_TIE1,         /* 13 */
   G_TIEd,         /* 14 */
   G_TIED,         /* 15 */
};

struct node
{

//for the gates
        char value; 
        bool flag;
//for the event list
        int data;
        node *link;
};

////////////////////////////////////////////////////////////////////////
// circuit class
////////////////////////////////////////////////////////////////////////

class circuit
{
    // circuit information
    int numgates;       // total number of gates (faulty included)
    int maxlevels;      // number of levels in gate level ckt
    int numInputs;	// number of inputs
    int numOutputs;	// number of outputs
    unsigned char *gtype;	// gate type
    short *fanin;		// number of fanins of gate
    short *fanout;		// number of fanouts of gate
    int *levelNum;		// level number of gate
    int **faninlist;		// fanin list of gate
    int **faninlist_copy;		// fanin list of gate
    int **fanoutlist;		// fanout list of gate

    int **FL_UNCOL_IN;
    int **FL_UNCOL_OUT;
    int **FL_COL_IN;
    int **FL_COL_OUT;

    /*char **store_input;
    char **store_output;
    char **store_dff;
    char *store_final_next_state;*/

    // you may wish to add more private variables here
    
    node *gate;
    //node *objective;
    node **eventlist;
    node **dfrontierA;
    char *pattern_buffer;
    int *pout, *dff;
    int numDFFs;

    public:
    circuit(char *);            // constructor

    // you may wish to add more function declarations (or public variables) here
    void add_event (int gate_number, int level_number ); 
    void add_dfrontierA (int gate_number ); 
    int check_valid_dfrontier(node *gate,int** faninlist,int** faninlist_copy,short* fanin,node** dfrontierA );
    void logic_simulate(int level_number, node *gate, char *pattern_buffer, int* levelNum, short* fanin, int** faninlist, short* fanout, int** fanoutlist, unsigned char* gtype);
    void evaluate(int j, unsigned char* gtype, node* gate, char* pattern_buffer, short* fanin, int** faninlist );
    void collapse_faults(int** FL_UNCOL_IN, int**FL_UNCOL_OUT, int** FL_COL_IN, int** FL_COL_OUT, short int* fanin, unsigned char* gtype, int gatenum);
    int PODEM_Recursion(int total_levels,int INP,int ST,int fs, int l, int count, int numOutputs,node* gate, int* pout, int** FL_COL_IN, node** dfrontierA,node** eventlist,int** faninlist,int** faninlist_copy, short* fanin, int** fanoutlist,short* fanout, char* pattern_buffer);
    node* getObjective(int INP, int ST, int fs, int l, node* gate, int** faninlist, short int* fanin, unsigned char* gtype, int** FL_COL_IN, node** dfrontierA);
    node* backtrace(int g, char v, node* gate, int** faninlist, short* fanin, unsigned char* gtype );
    int logicSimulate_and_imply(int total_levels, node *gate, char *pattern_buffer, int* levelNum, short* fanin, int** faninlist, short* fanout, int** fanoutlist, unsigned char* gtype);

};

////////////////////////////////////////////////////////////////////////
// constructor: reads in the *.lev file and builds basic data structure
// 		for the gate-level ckt
////////////////////////////////////////////////////////////////////////
circuit::circuit(char *cktName)
{
    FILE *inFile;
    FILE *outFile;
    FILE *outFile1;
    FILE *outFile2;

    char fName[40];
    int i, j, count;
    char c;
    int gatenum, junk;
    int f1, f2, f3;

    int lines, bit_count, vec_junk;

    strcpy(fName, cktName);
    strcat(fName, ".lev");
    inFile = fopen(fName, "r");
    if (inFile == NULL)
    {
	fprintf(stderr, "Can't open .lev file\n");
	exit(-1);
    }

    strcpy(fName, cktName);
    strcat(fName, ".vec");
    outFile = fopen(fName, "w");
    if (outFile == NULL)
    {
	fprintf(stderr, "Can't open .vec file\n");
	exit(-1);
    }

    strcpy(fName, cktName);
    strcat(fName, ".det");
    outFile1 = fopen(fName, "w");
    if (outFile1 == NULL)
    {
	fprintf(stderr, "Can't open .det file\n");
	exit(-1);
    }

    strcpy(fName, cktName);
    strcat(fName, ".undet");
    outFile2 = fopen(fName, "w");
    if (outFile2 == NULL)
    {
	fprintf(stderr, "Can't open .undet file\n");
	exit(-1);
    }

    numgates = maxlevels = 0;
    numInputs = numOutputs = 0;

    fscanf(inFile, "%d", &count);	// number of gates
    fscanf(inFile, "%d", &junk);	// skip the second line

    // allocate space for gates data structure
    gtype = new unsigned char[count+2];
    fanin = new short[count];
    fanout = new short[count];
    levelNum = new int[count];
    faninlist = new int * [count];
    faninlist_copy = new int * [count];
    fanoutlist = new int * [count];

    FL_UNCOL_IN = new int * [count];
    FL_UNCOL_OUT = new int * [count];
    FL_COL_IN = new int * [count];
    FL_COL_OUT = new int * [count];

    for (i=1;i<count;i++)
    {
    FL_UNCOL_IN[i] = new int[2];
    FL_UNCOL_OUT[i] = new int[2];
    FL_COL_IN[i] = new int[2];
    FL_COL_OUT[i] = new int[2];
       for(j=0;j<2;j++)
        {
         FL_UNCOL_IN[i][j] = 0;
         FL_UNCOL_OUT[i][j] = 0;
         FL_COL_IN[i][j] = 0;
         FL_COL_OUT[i][j] = 0;
        }
    }

    gate = new node[count+2];
    pout = new int[count];
    dff = new int[count];

    // now read in the circuit
    for (i=1; i<count; i++)
    {

	fscanf(inFile, "%d", &gatenum);
	fscanf(inFile, "%d", &f1);
	fscanf(inFile, "%d", &f2);
	fscanf(inFile, "%d", &f3);

	numgates++;
	gtype[gatenum] = (unsigned char) f1;
	if (gtype[gatenum] > 13)
	    printf("gate %d is an unimplemented gate type\n", gatenum);
	else if (gtype[gatenum] == G_INPUT)
	    numInputs++;
	else if (gtype[gatenum] == G_OUTPUT)
            {
	    numOutputs++;
            pout[numOutputs]=gatenum; 
	    }
        else if (gtype[gatenum] == G_DFF)
            {
            numDFFs++;
            dff[numDFFs]=gatenum;  
            }


	f2 = (int) f2;
        //levelNum[gatenum] = f2;
        levelNum[gatenum] = (f2+5)/5;	


	if (f2 >= (maxlevels))
	    maxlevels = f2 + 5;

        //initializing the output of all gates to X and flags to 0
        gate[gatenum].value = 'X';
        gate[gatenum].flag = 0; 

	fanin[gatenum] = (int) f3;
	// now read in the faninlist
	faninlist[gatenum] = new int[fanin[gatenum]];
	faninlist_copy[gatenum] = new int[fanin[gatenum]];
	for (j=0; j<fanin[gatenum]; j++)
	{
	    fscanf(inFile, "%d", &f1);
	    faninlist[gatenum][j] = (int) f1;
	    faninlist_copy[gatenum][j] = (int) f1;
	}

	for (j=0; j<fanin[gatenum]; j++) // followed by samethings
	    fscanf(inFile, "%d", &junk);

	// read in the fanout list
	fscanf(inFile, "%d", &f1);
	fanout[gatenum] = (int) f1;

	fanoutlist[gatenum] = new int[fanout[gatenum]];
	for (j=0; j<fanout[gatenum]; j++)
	{
	    fscanf(inFile, "%d", &f1);
	    fanoutlist[gatenum][j] = (int) f1;
	}

        collapse_faults(FL_UNCOL_IN,FL_UNCOL_OUT,FL_COL_IN,FL_COL_OUT,fanin,gtype,gatenum);

	// skip till end of line
	while ((c = getc(inFile)) != '\n' && c != EOF)
	    ;
    }	// for (i...)
    fclose(inFile);

    gtype[count]= (unsigned char) 14;   //TIEd for injecting d(0/1)
    gtype[count+1]= (unsigned char) 15; //TIED for injecting D(1/0)
    gate[count].value = 'd';
    gate[count].flag = 0; 
    gate[count+1].value = 'D';
    gate[count+1].flag = 0;


    int COL_faults_count=0;
    int UNCOL_faults_count=0;
    for(i=1;i<count;i++)
       {
       UNCOL_faults_count=UNCOL_faults_count+FL_UNCOL_IN[i][0]+FL_UNCOL_IN[i][1]+FL_UNCOL_OUT[i][0]+FL_UNCOL_OUT[i][1];
       COL_faults_count=COL_faults_count+FL_COL_IN[i][0]+FL_COL_IN[i][1]+FL_COL_OUT[i][0]+FL_COL_OUT[i][1];
       }

//PRINTING COLLAPSED FAULT LIST
printf("\nCOLLAPSED FAULT LIST\n");
    for(i=1;i<count;i++)
       {
       for(j=0;j<FL_COL_IN[i][0];j++)
         {
         printf("GATE %d INP %d s-a-0\n",i,j); 
         }
       for(j=0;j<FL_COL_IN[i][1];j++)
         {
         printf("GATE %d INP %d s-a-1\n",i,j); 
         }
       for(j=0;j<FL_COL_OUT[i][0];j++)
         {
         printf("GATE %d OUT s-a-0\n",i); 
         }
       for(j=0;j<FL_COL_OUT[i][1];j++)
         {
         printf("GATE %d OUT s-a-1\n",i); 
         }
       }

    int total_levels = maxlevels/5; 

    printf("Successfully read in circuit:\n");
    printf("\t%d PIs.\n", numInputs);
    printf("\t%d POs.\n", numOutputs);
    printf("\t%d Dffs.\n", numDFFs);
    printf("\t%d total number of gates\n", numgates);
    printf("\t%d levels in the circuit.\n", total_levels);
    printf("\t%d Total Number of Uncollapsed Faults \n",UNCOL_faults_count );
    printf("\t%d Total Number of Collapsed Faults \n",COL_faults_count );

    // (LOGIC SIMULATOR CODE STARTS HERE)
    
    // allocate space for event list and pattern buffer
    eventlist = new node*[total_levels+1];
    dfrontierA = new node*[1];



    for(i=0;i<total_levels+1;i++)
    eventlist[i]=NULL;

    dfrontierA[0] = NULL;


    pattern_buffer = new char[numInputs+2];

    for(i=1;i<=numInputs;i++)
    pattern_buffer[i]='X';

//ATPG
    fprintf(outFile, "%d\n", numInputs);	// number of gates

int fs,l,result, INP,ST,dcount;

INP=0;
ST=0;
result=0;
dcount=0;
int tempIN,temp,tempOUT;
for(fs=1;fs<count;fs++)
  {


       //S-A-0 Faults at gate inputs (FL_COL_IN[fs][0])
       for(l=0;l<FL_COL_IN[fs][0];l++)  
            {
             		printf("\nATPG for GATE %d INP %d s-a-0\n",fs,l);

            		//Initializing
            		for(i=0;i<total_levels+1;i++)
            		eventlist[i]=NULL;

            		dfrontierA[0] = NULL;

             		for(i=1;i<count;i++)
             		  {
               			 gate[i].value = 'X';
               			 gate[i].flag = 0; 
              		  }

             		for(i=1;i<=numInputs;i++)
               		  {
               			 pattern_buffer[i] = 'X';
                          }

                        //S-A-0 Faults at gate inputs 
                        INP=1;ST=0;

                        //Tying the gate input with S-A-0 fault to 'D' i.e. 1/0
                	tempIN=faninlist[fs][l];
                	faninlist[fs][l]=count+1;
                	add_event(fs,levelNum[fs]);
                	gate[fs].flag=1;

             		//If fault effect is observed at a PO, return 1
            		//result = PODEM_Recursion(C);
            		result=PODEM_Recursion(total_levels,INP,ST,fs,l,count,numOutputs,gate,pout,FL_COL_IN,dfrontierA,eventlist,faninlist,faninlist_copy, fanin, fanoutlist,fanout, pattern_buffer);

             		if(result==1)
               		   {
               			dcount++;
               			//print out values at the PIs; 
               			printf("\n Pattern to detect fault GATE %d INP %d s-a-0\n",fs,l);
                                fprintf(outFile1, "GATE %d INP %d s-a-0\n",fs,l);
                  		for(i=1;i<=numInputs;i++)
                   		   {
                   			printf("%c",gate[i].value);
                            fprintf(outFile, "%c", gate[i].value);	
                   		   }
                   		printf("\n");
                        fprintf(outFile, "\n");	
               		   }
              		else
               		   {
               			//Print fault f is untestable
               			printf("\n fault GATE %d INP %d s-a-0 is untestable\n",fs,l);
                                fprintf(outFile2, "GATE %d INP %d s-a-0\n",fs,l);
               		   }


                        //Reverting back the tied circutry to original for next fault
                        faninlist[fs][l]=faninlist_copy[fs][l];

          }



       //S-A-1 Faults at gate inputs (FL_COL_IN[fs][1])
      for(l=0;l<FL_COL_IN[fs][1];l++)  
          {
             		printf("\nATPG for GATE %d INP %d s-a-1\n",fs,l);

            		//Initializing
             		for(i=0;i<total_levels+1;i++)
            		eventlist[i]=NULL;

             		dfrontierA[0] = NULL;

             		for(i=1;i<count;i++)
               		  {
                		gate[i].value = 'X';
                		gate[i].flag = 0; 
               		  }

             		for(i=1;i<=numInputs;i++)
               		  {
                		pattern_buffer[i] = 'X';
               		  }


                        //S-A-1 Faults at gate inputs 
                         INP=1;ST=1;

                        //Tying the gate input with S-A-1 fault to 'd' i.e. 0/1
                	tempIN=faninlist[fs][l];
                	faninlist[fs][l]=count;
                	add_event(fs,levelNum[fs]);
                	gate[fs].flag=1;

                        //If fault effect is observed at a PO, return 1
                        //result = PODEM_Recursion(C);
                        result=PODEM_Recursion(total_levels,INP,ST,fs,l,count,numOutputs,gate,pout,FL_COL_IN,dfrontierA,eventlist,faninlist,faninlist_copy, fanin, fanoutlist,fanout, pattern_buffer);

                       if(result==1)
                          {
               			dcount++;
               			//print out values at the PIs; 
               			printf("\n Pattern to detect fault GATE %d INP %d s-a-1\n",fs,l);
                                fprintf(outFile1, "GATE %d INP %d s-a-1\n",fs,l);
                 		for(i=1;i<=numInputs;i++)
                  		    {
                  			 printf("%c",gate[i].value);
                            fprintf(outFile, "%c", gate[i].value);	
                  		    }
                  		printf("\n");
                        fprintf(outFile, "\n");	
               		  }
              	       else
              		  {
               			//Print fault f is untestable
              			 printf("\n fault GATE %d INP %d s-a-1 is untestable\n",fs,l);
                                 fprintf(outFile2, "GATE %d INP %d s-a-1\n",fs,l);
              		  }

                        //Reverting back the tied circutry to original for next fault
              		faninlist[fs][l]=faninlist_copy[fs][l];

          }


     //S-A-0 Faults at gate output (FL_COL_OUT[fs][0])
     for(l=0;l<FL_COL_OUT[fs][0];l++)  
           {
             		printf("\nATPG for GATE %d OUT s-a-0\n",fs);
             		for(i=0;i<total_levels+1;i++)
             		eventlist[i]=NULL;

             		dfrontierA[0] = NULL;

             		for(i=1;i<count;i++)
               		   {
                		gate[i].value = 'X';
                		gate[i].flag = 0; 
               		   }

             		for(i=1;i<=numInputs;i++)
               		   {
                		pattern_buffer[i] = 'X';
                           }

                        //S-A-0 Faults at gate output 
                        INP=0;ST=0;

                        //Tying the gate inputs with S-A-0 fault to 'D' i.e. 1/0
                	for(i=0;i<fanout[fs];i++)
                	        {
                    				 temp=fanoutlist[fs][i];
                    				 for(j=0;j<fanin[temp];j++)
                       					{
                        					if(fs==faninlist[temp][j])
                        						 {
                        				 		       tempOUT=faninlist[temp][j];
                         						       faninlist[temp][j]=count+1;
                          						       add_event(temp,levelNum[temp]);
                          					               gate[temp].flag=1;
                        						 }
                      					}
                 		 }

                        //If fault effect is observed at a PO, return 1
                        //result = PODEM_Recursion(C);
             		//printf("\nBefore First PODEM Recursion\n");
						
                        result=PODEM_Recursion(total_levels,INP,ST,fs,l,count,numOutputs,gate,pout,FL_COL_IN,dfrontierA,eventlist,faninlist,faninlist_copy, fanin, fanoutlist,fanout, pattern_buffer);

             		//printf("\nAfter First PODEM Recursion\n");

                        if(result==1)
                            {
                                 dcount++;
                                 //print out values at the PIs; 
                                 printf("\n Pattern to detect fault GATE %d OUT s-a-0\n",fs);
                                 fprintf(outFile1, "GATE %d OUT s-a-0\n",fs);
                                 for(i=1;i<=numInputs;i++)
                                     {
                                         printf("%c",gate[i].value);
                                         fprintf(outFile, "%c", gate[i].value);	
                                     }
                                 printf("\n");
                                 fprintf(outFile, "\n");	
                            }
                        else
                            {
                                 //Print fault f is untestable
                                 printf("\n fault GATE %d OUT s-a-0  is untestable\n",fs);
                                 fprintf(outFile2, "GATE %d OUT s-a-0\n",fs);
                            }


                        //Reverting back the tied circutry to original for next fault
               		 for(i=0;i<fanout[fs];i++)
                  		   {
                    					 temp=fanoutlist[fs][i];
                     					 for(j=0;j<fanin[temp];j++)
                      						 {
                          								 faninlist[temp][j]=faninlist_copy[temp][j];

                     						 }

                  	           }

          }


     //S-A-1 Faults at gate output (FL_COL_OUT[fs][1])
     for(l=0;l<FL_COL_OUT[fs][1];l++)  
           {
             		printf("\nATPG for GATE %d OUT s-a-1\n",fs);
             		for(i=0;i<total_levels+1;i++)
             		eventlist[i]=NULL;

             		dfrontierA[0] = NULL;

             		for(i=1;i<count;i++)
               		   {
                		gate[i].value = 'X';
                		gate[i].flag = 0; 
               		   }

             		for(i=1;i<=numInputs;i++)
               		   {
                		pattern_buffer[i] = 'X';
                           }

                        //S-A-1 Faults at gate output 
                        INP=0;ST=1;

                        //Tying the gate inputs with S-A-1 fault to 'd' i.e. 0/1
                	for(i=0;i<fanout[fs];i++)
                	        {
                    				 temp=fanoutlist[fs][i];
                    				 for(j=0;j<fanin[temp];j++)
                       					{
                        					if(fs==faninlist[temp][j])
                        						 {
                        				 		       tempOUT=faninlist[temp][j];
                         						       faninlist[temp][j]=count;
                          						       add_event(temp,levelNum[temp]);
                          					               gate[temp].flag=1;
                        						 }
                      					}
                 		 }

                        //If fault effect is observed at a PO, return 1
                        //result = PODEM_Recursion(C);
                        result=PODEM_Recursion(total_levels,INP,ST,fs,l,count,numOutputs,gate,pout,FL_COL_IN,dfrontierA,eventlist,faninlist,faninlist_copy, fanin, fanoutlist,fanout, pattern_buffer);

                        if(result==1)
                            {
                                 dcount++;
                                 //print out values at the PIs; 
                                 printf("\n Pattern to detect fault GATE %d OUT s-a-1\n",fs);
                                 fprintf(outFile1, "GATE %d OUT s-a-1\n",fs);
                                 for(i=1;i<=numInputs;i++)
                                     {
                                         printf("%c",gate[i].value);
                                         fprintf(outFile, "%c", gate[i].value);	
                                     }
                                 printf("\n");
                                 fprintf(outFile, "\n");	
                            }
                        else
                            {
                                 //Print fault f is untestable
                                 printf("\n fault GATE %d OUT s-a-1  is untestable\n",fs);
                                 fprintf(outFile2, "GATE %d OUT s-a-1\n",fs);
                            }


                        //Reverting back the tied circutry to original for next fault
               		 for(i=0;i<fanout[fs];i++)
                  		   {
                    					 temp=fanoutlist[fs][i];
                     					 for(j=0;j<fanin[temp];j++)
                      						 {
                          								 faninlist[temp][j]=faninlist_copy[temp][j];

                     						 }

                  	           }

          }

   }

    printf("\nTotal Number of Collapsed Faults=%d\n",COL_faults_count );
    printf("\nTotal Detected Faults=%d\n",dcount);
fprintf(outFile, "END");	
fclose(outFile);
fclose(outFile1);
fclose(outFile2);

}//Constructor ends

////////////////////////////////////////////////////////////////////////
// other member functions here
////////////////////////////////////////////////////////////////////////
node* circuit::backtrace(int g, char v, node* gate, int** faninlist, short* fanin, unsigned char* gtype )      
  {
    int i,j,k;
    i=g;
    int temp;
    int num_inversion=0;
//printf("\nInside Backtrace 1 i=%d\n",i);

    node *backtracePI;
    backtracePI = new node;
    while(gtype[i]!=G_INPUT)
       {
//printf("\nInside Backtrace 2 i=%d gtype=%d\n",i,gtype[i]);

          for(j=0;j<fanin[i];j++)
            {
//printf("\nInside Backtrace 3 fanin[%d]=%d \n",i,fanin[i]);

             if(gate[faninlist[i][j]].value=='X'||gate[faninlist[i][j]].value=='A'||gate[faninlist[i][j]].value=='a'||gate[faninlist[i][j]].value=='B'||gate[faninlist[i][j]].value=='b')
               {
//printf("\nInside Backtrace 4 faninlist[%d][%d]=%d faninlist[%d][%d].value=%c\n",i,j,faninlist[i][j],i,j,gate[faninlist[i][j]].value);

                if((gtype[i]==G_NOR)||(gtype[i]==G_NAND)||(gtype[i]==G_NOT)||(gtype[i]==G_XNOR))
                 {
                  num_inversion++;
//printf("\nInside Backtrace 5 num_inversion=%d\n",num_inversion);

                 }

                if(((gtype[i]==G_XOR)||(gtype[i]==G_XNOR))&&(fanin[i]==2))
                  {
                    for(k=0;k<fanin[i];k++)
                      {
                         if(gate[faninlist[i][k]].value=='1')
                         num_inversion++;
                      }
                  }

                 i=faninlist[i][j];
//printf("\nInside Backtrace 6 i=%d\n",i);
                 break;
               }
            }
       }

    temp=num_inversion%2;

    if(temp==0)
     {
       if(v=='0')
       backtracePI->value='0'; 
       else
       backtracePI->value='1';  
     }
     else
     {
       if(v=='0')
       backtracePI->value='1'; 
       else
       backtracePI->value='0';
     }

    backtracePI->data=i;

    return backtracePI;

} //end function


node* circuit::getObjective(int INP, int ST, int fs, int l, node* gate, int** faninlist, short int* fanin, unsigned char* gtype, int** FL_COL_IN, node** dfrontierA)      
  {
//printf("\nInside get Objective 1\n");

      node *objective;
      objective = new node;
      int i,num;
//If fault is not excited, return (g,v')
            if(INP==1)
               {
//printf("\nInside get Objective 2\n");

                if(gate[faninlist_copy[fs][l]].value=='X')
                  {
//printf("\nInside get Objective 3\n");

                  objective->data  = faninlist_copy[fs][l];
//printf("\nInside get Objective 4\n");

                  if(ST==0)
                  objective->value = '1';
                  else
                  objective->value = '0';
//printf("\nInside get Objective 5 objective->value=%c\n",objective->value);

                  return objective;
                  }
               }
             else
               {
                if(gate[fs].value=='X')
                  {
                  objective->data  = fs;
                  if(ST==0)
                  objective->value = '1';
                  else
                  objective->value = '0';

                  return objective;
                  }
               }

//printf("\nInside get Objective 6a\n");

//d=a gate in D-frontier; g=an input of d whose value is X; v=non-controlling value of d; return(g,v)
           node *current = dfrontierA[0];
           while(current != NULL)
	    {
//printf("\nInside get Objective 6\n");

              num = current->data;
                //printf("\n num=%d fanin[num]=%d\n",num,fanin[num]);

              for(i=0;i<fanin[num];i++)
                {
                //printf("\n value=%c\n",gate[faninlist[num][i]].value);
                if(gate[faninlist[num][i]].value=='X'||gate[faninlist[num][i]].value=='A'||gate[faninlist[num][i]].value=='a'||gate[faninlist[num][i]].value=='B'||gate[faninlist[num][i]].value=='b')
                  {
                           switch (gtype[num])
                            { //switch begins
			      //INPUT
		              case 1:
		              break;

                              //NAND gate
			      case 7:
                              objective->value='1'; 
			      break;

                              //AND gate
                              case 6:
                              objective->value='1';
			      break;

                              //OR gate
                              case 8:
                              objective->value='0';
			      break;

                              //NOR gate
                              case 9:
                              objective->value='0';
			      break; 

                              //XOR gate
			      case 3:
                              objective->value='0';
			      break;

                             //XNOR gate
			      case 4:
                              objective->value='0';
			      break; 

			      //NOT gate
                              case 10:
			      break;

                             //BUF gate
			      case 11:
			      break;

                              //TIE_1
                              case 12:
			      break;

                              //TIE_0
			      case 13:
			      break;

                              //OUTPUT
			      case 2:
			      break;

			      //D_FF
			      case 5:
			      break;

			   } //switch ends
                      objective->data  = faninlist[num][i];
                      //printf("\nObjective return %d\n",faninlist[num][i]);
                      return objective;

                  }//if loop ends
                }//for loop ends
              
              current = current->link;
	    }//while loop ends

}//function ends


 int circuit::PODEM_Recursion(int total_levels,int INP,int ST,int fs, int l, int count,int numOutputs,node* gate, int* pout,int** FL_COL_IN, node** dfrontierA,node** eventlist,int** faninlist,int** faninlist_copy, short* fanin, int** fanoutlist,short* fanout, char* pattern_buffer)
      {

         node *objective;
         objective = new node;
         node *backtracePI;
         backtracePI =new node;
         int result=0;
         int i,j,g,pi;
         char v,u;
         int tempIN, tempOUT, temp;
         int df_check=0; 
    
         int flag=0;
         int num;
         int check=0;

//To check if fault is observed at POs, also taking into consideration the situation where the fault is at PO
          if(INP==1)
            {

             		           if(ST==0)
               		              {
               		               if(gate[faninlist_copy[fs][l]].value=='0')
                                            {
                                         		return 0; 
                                            }
                                      }
                                   else
                                      {
               		               if(gate[faninlist_copy[fs][l]].value=='1')
                                           {
                 		                        return 0; 
                                          }
                                      }

                             if(gtype[fs]!=2)
                                 {
               				//If fault effect is observed at a PO, return 1
               				for(i=1;i<=numOutputs;i++)
                				{
               		  		           if(gate[pout[i]].value=='D' || gate[pout[i]].value=='d')
                 			           return 1;
               			                }
                                 }
                             else
                                {

             		           if(ST==0)
               		              {
               		               if(gate[faninlist_copy[fs][l]].value=='1')
                                            {
                                         		return 1; 
                                            }
                                      }
                                   else
                                      {
               		               if(gate[faninlist_copy[fs][l]].value=='0')
                                           {
                 		                        return 1; 
                                          }
                                      }
                               }
            }
          else
            {
             		if(ST==0)
               		   {
                		if(gate[fs].value=='1')
                                  {
               		               for(i=0;i<fanout[fs];i++)
                		            {
                    				 temp=fanoutlist[fs][i];
                                                 if(gtype[temp]==2)
                                                 return 1;
                                            }
                                  }

                		if(gate[fs].value=='0')
                                  {
                                                 return 0;
                                  }
                           }
                        else
                           {
                		if(gate[fs].value=='0')
                                  {
               		               for(i=0;i<fanout[fs];i++)
                		            {
                    				 temp=fanoutlist[fs][i];
                                                 if(gtype[temp]==2)
                                                 return 1;
                                            }
                                  }
                		if(gate[fs].value=='1')
                                  {
                                                 return 0;
                                  }
                           }

               		for(i=1;i<=numOutputs;i++)
                		{
                                  flag=0;
                                        for(j=0;j<fanout[fs];j++)
                		            {
                    				 temp=fanoutlist[fs][j];
                                                 if(temp==pout[i])
                                                  {
                                                    flag=1;
                                                    break;
                                                  }
                                            }
 
               		           if((flag==0) && (gate[pout[i]].value=='D' || gate[pout[i]].value=='d'))
                 	           return 1;
               		        }


            }

	
          //Check if Objective Violated (1)Fault excited to undesired value or (2) Fault excited, but D-Frontier empty. If fault excited, put in dfrontier. If objectives violated, return 0
          if(INP==1)
            {

             		if(ST==0)
               		   {
                		if(gate[faninlist_copy[fs][l]].value=='0')
                                {
                 		return 0; 
                                }

                		if((gate[faninlist_copy[fs][l]].value=='1') && (gate[fs].value=='X'||gate[fs].value=='a'||gate[fs].value=='A'||gate[fs].value=='b'||gate[fs].value=='B'))
                			{
                				add_dfrontierA(fs);
                			}

                		if((gate[faninlist_copy[fs][l]].value=='1') && (gate[fs].value=='D'||gate[fs].value=='d'||gate[fs].value=='1'||gate[fs].value=='0'))
                			{
						if(dfrontierA[0]==NULL)
						return 0;
                			}
               		   }
                        else
                           {

                		if(gate[faninlist_copy[fs][l]].value=='1')
                                 {
                		 return 0; 
                                 }


                		if((gate[faninlist_copy[fs][l]].value=='0') && (gate[fs].value=='X'||gate[fs].value=='a'||gate[fs].value=='A'||gate[fs].value=='b'||gate[fs].value=='B'))
               				{
                				add_dfrontierA(fs);
                			}

               		if((gate[faninlist_copy[fs][l]].value=='0') && (gate[fs].value=='D'||gate[fs].value=='d'||gate[fs].value=='1'||gate[fs].value=='0'))
                			{
						if(dfrontierA[0]==NULL)
						return 0;
                			}
              		   }
             }
          else
             {
          		if(ST==0)
              		  {
                		if(gate[fs].value=='0')
                 		return 0; 

                                df_check=0;
                		for(i=0;i<fanout[fs];i++)
                		  {
                   			if((gate[fs].value=='1')&&((gate[fanoutlist[fs][i]].value=='X')||(gate[fanoutlist[fs][i]].value=='A')||(gate[fanoutlist[fs][i]].value=='a')||(gate[fanoutlist[fs][i]].value=='B')||(gate[fanoutlist[fs][i]].value=='b')))
                   				 {
                   					 add_dfrontierA(fanoutlist[fs][i]);
                    				 }						
                   			if((gate[fs].value=='1')&&((gate[fanoutlist[fs][i]].value=='D')||(gate[fanoutlist[fs][i]].value=='d')||(gate[fanoutlist[fs][i]].value=='1')||(gate[fanoutlist[fs][i]].value=='0')))
                   				 {
                                                         df_check++;
                    				 }
                 		 }
                                if(df_check==fanout[fs])
                                  {
                                      if(dfrontierA[0]==NULL)
                                      return 0;
                                  }
              		  }
              	       else
              		 {
               			 if(gate[fs].value=='1')
                		 return 0; 

                                df_check=0;
                		 for(i=0;i<fanout[fs];i++)
                  		   {
                  			 if((gate[fs].value=='0')&&((gate[fanoutlist[fs][i]].value=='X')||(gate[fanoutlist[fs][i]].value=='A')||(gate[fanoutlist[fs][i]].value=='a')||(gate[fanoutlist[fs][i]].value=='B')||(gate[fanoutlist[fs][i]].value=='b')))
                   				 {
                    					 add_dfrontierA(fanoutlist[fs][i]);

                   				 }
                   			if((gate[fs].value=='0')&&((gate[fanoutlist[fs][i]].value=='D')||(gate[fanoutlist[fs][i]].value=='d')||(gate[fanoutlist[fs][i]].value=='1')||(gate[fanoutlist[fs][i]].value=='0')))
                   				 {
                                                         df_check++;
                    				 }
                 		 }
                                if(df_check==fanout[fs])
                                  {
                                      if(dfrontierA[0]==NULL)
                                      return 0;
                                  }

                          }
              }


//To check if the D-frontier contains valid data to prevent getObjective from returning corrupt data
          if(INP==1)
            {
             		if(ST==0)
               		   {
                		if(gate[faninlist_copy[fs][l]].value=='1')
                                {
								check=0;
                                check=check_valid_dfrontier(gate,faninlist,faninlist_copy,fanin,dfrontierA );
								if(check==0)
								return 0;
                                }
                        }
					    else
                        {
                		if(gate[faninlist_copy[fs][l]].value=='0')
                               {
								check=0;
                                check=check_valid_dfrontier(gate,faninlist,faninlist_copy,fanin,dfrontierA );
								if(check==0)
								return 0;
                              }
                         }
          }
          else
          {
             		if(ST==0)
               		   {
                		if(gate[fs].value=='1')
                                {
								check=0;
                                check=check_valid_dfrontier(gate,faninlist,faninlist_copy,fanin,dfrontierA );
								if(check==0)
								return 0;
                                }
                        }
					    else
                        {
                		if(gate[fs].value=='0')
                                {
								check=0;
                                check=check_valid_dfrontier(gate,faninlist,faninlist_copy,fanin,dfrontierA );
								if(check==0)
								return 0;
                               }
                          }
          }


            //(g,v)=getObjective(C);
            objective=getObjective(INP, ST, fs, l, gate, faninlist, fanin, gtype, FL_COL_IN, dfrontierA);

            g=objective->data;
            v=objective->value;

            //(pi,u)=backtrace(g,v);
            backtracePI=backtrace(g, v, gate, faninlist, fanin, gtype );


            pi=backtracePI->data;
            u=backtracePI->value;
            pattern_buffer[pi]=u;

            //logicSimulate_and_imply(pi,u);
            logicSimulate_and_imply(total_levels, gate, pattern_buffer, levelNum, fanin, faninlist, fanout, fanoutlist, gtype);


//If the Objective is excited to opposite value, return 0 to avoid further iteration
            if(gate[g].value==v || gate[g].value=='X'||gate[g].value=='A'||gate[g].value=='a'||gate[g].value=='b'||gate[g].value=='B'||gate[g].value=='D'||gate[g].value=='d')
              {
            dfrontierA[0]=NULL;

                 for(i=1;i<count;i++)
                   {

                     if(gate[i].value=='X'||gate[i].value=='A'||gate[i].value=='a'||gate[i].value=='B'||gate[i].value=='b')
                       {

                          for(j=0;j<fanin[i];j++)
                            {
                              if(gate[faninlist[i][j]].value=='D'|| gate[faninlist[i][j]].value=='d')
                                 {
                                 add_dfrontierA(i);
                                 }
                            }
                       }
                   }

            //result=PODEM_Recursion(C);
            result=PODEM_Recursion(total_levels,INP,ST, fs,  l, count, numOutputs, gate,  pout, FL_COL_IN,  dfrontierA, eventlist, faninlist, faninlist_copy,  fanin,  fanoutlist, fanout,  pattern_buffer);

            if(result==1)
               return 1;

              }
              else
              {
            //Back Track
              return 0;
              }
            //if(result==1)
             //  return 1;

            //Back Track

            if(u=='0')
              u='1';
            else
              u='0';

            pattern_buffer[pi]=u;
            //logicSimulate_and_imply(pi,u');
            logicSimulate_and_imply(total_levels, gate, pattern_buffer, levelNum, fanin, faninlist, fanout, fanoutlist, gtype);

            //result=PODEM_Recursion(C);
            result=PODEM_Recursion(total_levels,INP,ST,fs,l, count,numOutputs,gate,pout,FL_COL_IN,dfrontierA,eventlist,faninlist,faninlist_copy, fanin, fanoutlist,fanout, pattern_buffer);

            if(result==1)
               return 1;

            //Bad Decision made at an earlier step, reset pi
            pattern_buffer[pi]='X';
            //logicSimulate_and_imply(pi,X);

            //Reverting the tied circuitry to original state
            if(INP==1)
              {
              faninlist[fs][l]=faninlist_copy[fs][l];
              }
            else
              {
               		 for(i=0;i<fanout[fs];i++)
                  		   {
                    					 temp=fanoutlist[fs][i];
                     					 for(j=0;j<fanin[temp];j++)
                      						 {
                          								 faninlist[temp][j]=faninlist_copy[temp][j];
                     						 }

                  	           }

              }

            logicSimulate_and_imply(total_levels, gate, pattern_buffer, levelNum, fanin, faninlist, fanout, fanoutlist, gtype);
            return 0;
             
   }

//Function to schedule gate onto the eventwheel
void circuit::add_event (int gate_number, int level_number )
 	{
	node *current = new node;
	current->data =gate_number; 
	current->link = eventlist[level_number];
        eventlist[level_number] = current;
        }

//Function to add gate in dfrontier 
void circuit::add_dfrontierA (int gate_number )
 	{
	node *current = new node;
	current->data =gate_number; 
	current->link = dfrontierA[0];
    dfrontierA[0] = current;
    }

//Function to check that the D-Frontier contains valid data
int circuit::check_valid_dfrontier(node *gate,int** faninlist,int** faninlist_copy,short* fanin,node** dfrontierA )
       {
                 int i,num;
                 int check=0;
           							node *current = dfrontierA[0];
          								 while(current != NULL)
	        								{
              									num = current->data;

              									for(i=0;i<fanin[num];i++)
               									 {
                								if(gate[faninlist[num][i]].value=='X'||gate[faninlist[num][i]].value=='A'||gate[faninlist[num][i]].value=='a'||gate[faninlist[num][i]].value=='B'||gate[faninlist[num][i]].value=='b')
                  										{
                  											check=1; 
                 										 }//if loop ends
               									 }//for loop ends
              
              									current = current->link;
	       									 }//while loop ends

                     return check;
      }

//Logic simulator function
void circuit::logic_simulate(int level_number, node *gate, char *pattern_buffer, int* levelNum, short* fanin, int** faninlist, short* fanout, int** fanoutlist, unsigned char* gtype)
        {
           node *delNode = new node;
           char temp;
           int i, num,m,temp2;
//printf("\nInside Logic Simulator 1 \n");

           while(eventlist[level_number] != NULL)
	   {
           num = eventlist[level_number]->data;
           temp = gate[num].value;

           evaluate(num, gtype, gate, pattern_buffer, fanin, faninlist );
           gate[num].flag = 0;


           if((temp != gate[num].value)&&gtype[num]!=2)
                {
                    for(i=0; i<fanout[num]; i++)
                    { 
                      if(gate[fanoutlist[num][i]].flag == 0)
		      { 

                                                                temp2=fanoutlist[num][i];
                                                                for(m=0;m<fanin[temp2];m++)
                                                                  {
                                                                     //If the fanin doesn't have the tied 'd' or 'D' gate
                                                                     if(num==faninlist[temp2][m])
                                                                         {
                                                                          add_event(fanoutlist[num][i], levelNum[fanoutlist[num][i]]); 
                                                                          gate[fanoutlist[num][i]].flag = 1;
                                                                        }
                                                                  }
                      }
                    }
                 }
	   delNode = eventlist[level_number];
	   eventlist[level_number] = eventlist[level_number]->link;
	   delete delNode;
	   }
        }


int circuit::logicSimulate_and_imply(int total_levels, node *gate, char *pattern_buffer, int* levelNum, short* fanin, int** faninlist, short* fanout, int** fanoutlist, unsigned char* gtype)
         {
                        int i,j,k;

	       		for (j=1; j<=numInputs; j++)
	      			 {
                 			  if(gate[j].value != pattern_buffer[j])
                   				  { 
                     				    add_event(j,1); 
                     				    gate[j].flag = 1;
                     				  }
               			 } 

               		//For each level
              		for(k=1;k<=total_levels;k++)            
              			 {
                 		//Calling the Logic Simulate function
                 		logic_simulate(k, gate, pattern_buffer, levelNum, fanin, faninlist, fanout, fanoutlist, gtype);
              			 }

        } //function ends

void circuit::collapse_faults(int** FL_UNCOL_IN, int**FL_UNCOL_OUT, int** FL_COL_IN, int** FL_COL_OUT, short int* fanin, unsigned char* gtype, int gatenum)
   {

         switch (gtype[gatenum])
              { //switch begins
			 //INPUT
		          case 1:
		          FL_UNCOL_IN[gatenum][0]=0;
		          FL_UNCOL_OUT[gatenum][0]=1;
		          FL_UNCOL_IN[gatenum][1]=0;
		          FL_UNCOL_OUT[gatenum][1]=1;

		          FL_COL_IN[gatenum][0]=0;
		          FL_COL_OUT[gatenum][0]=1;
		          FL_COL_IN[gatenum][1]=0;
		          FL_COL_OUT[gatenum][1]=1;
		          break;

                          //NAND gate
			  case 7:
		          FL_UNCOL_IN[gatenum][0]=fanin[gatenum];
		          FL_UNCOL_OUT[gatenum][0]=1;
		          FL_UNCOL_IN[gatenum][1]=fanin[gatenum];
		          FL_UNCOL_OUT[gatenum][1]=1;

		          FL_COL_IN[gatenum][0]=0;
		          FL_COL_OUT[gatenum][0]=1;
		          FL_COL_IN[gatenum][1]=fanin[gatenum];
		          FL_COL_OUT[gatenum][1]=1;
			  break;

                          //AND gate
                          case 6:
		          FL_UNCOL_IN[gatenum][0]=fanin[gatenum];
		          FL_UNCOL_OUT[gatenum][0]=1;
		          FL_UNCOL_IN[gatenum][1]=fanin[gatenum];
		          FL_UNCOL_OUT[gatenum][1]=1;

		          FL_COL_IN[gatenum][0]=0;
		          FL_COL_OUT[gatenum][0]=1;
		          FL_COL_IN[gatenum][1]=fanin[gatenum];
		          FL_COL_OUT[gatenum][1]=1;
                          break;

                          //OR gate
                          case 8:
		          FL_UNCOL_IN[gatenum][0]=fanin[gatenum];
		          FL_UNCOL_OUT[gatenum][0]=1;
		          FL_UNCOL_IN[gatenum][1]=fanin[gatenum];
		          FL_UNCOL_OUT[gatenum][1]=1;

		          FL_COL_IN[gatenum][0]=fanin[gatenum];
		          FL_COL_OUT[gatenum][0]=1;
		          FL_COL_IN[gatenum][1]=0;
		          FL_COL_OUT[gatenum][1]=1;
                          break;

                          //NOR gate
                          case 9:
		          FL_UNCOL_IN[gatenum][0]=fanin[gatenum];
		          FL_UNCOL_OUT[gatenum][0]=1;
		          FL_UNCOL_IN[gatenum][1]=fanin[gatenum];
		          FL_UNCOL_OUT[gatenum][1]=1;

		          FL_COL_IN[gatenum][0]=fanin[gatenum];
		          FL_COL_OUT[gatenum][0]=1;
		          FL_COL_IN[gatenum][1]=0;
		          FL_COL_OUT[gatenum][1]=1;
                          break;

                          //XOR gate
			  case 3:
		          FL_UNCOL_IN[gatenum][0]=fanin[gatenum];
		          FL_UNCOL_OUT[gatenum][0]=1;
		          FL_UNCOL_IN[gatenum][1]=fanin[gatenum];
		          FL_UNCOL_OUT[gatenum][1]=1;

		          FL_COL_IN[gatenum][0]=fanin[gatenum];
		          FL_COL_OUT[gatenum][0]=1;
		          FL_COL_IN[gatenum][1]=fanin[gatenum];
		          FL_COL_OUT[gatenum][1]=1;
                          break;

                          //XNOR gate
			  case 4:
		          FL_UNCOL_IN[gatenum][0]=fanin[gatenum];
		          FL_UNCOL_OUT[gatenum][0]=1;
		          FL_UNCOL_IN[gatenum][1]=fanin[gatenum];
		          FL_UNCOL_OUT[gatenum][1]=1;

		          FL_COL_IN[gatenum][0]=fanin[gatenum];
		          FL_COL_OUT[gatenum][0]=1;
		          FL_COL_IN[gatenum][1]=fanin[gatenum];
		          FL_COL_OUT[gatenum][1]=1;
                          break;

			  //NOT gate
                          case 10:
		          FL_UNCOL_IN[gatenum][0]=1;
		          FL_UNCOL_OUT[gatenum][0]=1;
		          FL_UNCOL_IN[gatenum][1]=1;
		          FL_UNCOL_OUT[gatenum][1]=1;

		          FL_COL_IN[gatenum][0]=0;
		          FL_COL_OUT[gatenum][0]=1;
		          FL_COL_IN[gatenum][1]=0;
		          FL_COL_OUT[gatenum][1]=1;
                          break;

                          //BUF gate
			  case 11:
		          FL_UNCOL_IN[gatenum][0]=1;
		          FL_UNCOL_OUT[gatenum][0]=1;
		          FL_UNCOL_IN[gatenum][1]=1;
		          FL_UNCOL_OUT[gatenum][1]=1;

		          FL_COL_IN[gatenum][0]=0;
		          FL_COL_OUT[gatenum][0]=1;
		          FL_COL_IN[gatenum][1]=0;
		          FL_COL_OUT[gatenum][1]=1;
                          break;

                          //TIE_1
                          case 12:
		          FL_UNCOL_IN[gatenum][0]=0;
		          FL_UNCOL_OUT[gatenum][0]=1;
		          FL_UNCOL_IN[gatenum][1]=0;
		          FL_UNCOL_OUT[gatenum][1]=0;

		          FL_COL_IN[gatenum][0]=0;
		          FL_COL_OUT[gatenum][0]=1;
		          FL_COL_IN[gatenum][1]=0;
		          FL_COL_OUT[gatenum][1]=0;
		          break;

                          //TIE_0
			  case 13:
		          FL_UNCOL_IN[gatenum][0]=0;
		          FL_UNCOL_OUT[gatenum][0]=0;
		          FL_UNCOL_IN[gatenum][1]=0;
		          FL_UNCOL_OUT[gatenum][1]=1;

		          FL_COL_IN[gatenum][0]=0;
		          FL_COL_OUT[gatenum][0]=0;
		          FL_COL_IN[gatenum][1]=0;
		          FL_COL_OUT[gatenum][1]=1;
                          break;

                          //OUTPUT
			  case 2:
		          FL_UNCOL_IN[gatenum][0]=1;
		          FL_UNCOL_OUT[gatenum][0]=0;
		          FL_UNCOL_IN[gatenum][1]=1;
		          FL_UNCOL_OUT[gatenum][1]=0;

		          FL_COL_IN[gatenum][0]=1;
		          FL_COL_OUT[gatenum][0]=0;
		          FL_COL_IN[gatenum][1]=1;
		          FL_COL_OUT[gatenum][1]=0;
                          break;

			  //D_FF
			  case 5:
		          FL_UNCOL_IN[gatenum][0]=1;
		          FL_UNCOL_OUT[gatenum][0]=1;
		          FL_UNCOL_IN[gatenum][1]=1;
		          FL_UNCOL_OUT[gatenum][1]=1;

		          FL_COL_IN[gatenum][0]=1;
		          FL_COL_OUT[gatenum][0]=1;
		          FL_COL_IN[gatenum][1]=1;
		          FL_COL_OUT[gatenum][1]=1;
			  break;

		   } //switch ends


		} //function ends


//Gate evaluations for logic simulator
//d: 0/1  D:1/0  a:0/X   A:1/X  b:X/0   B:X/1
void circuit::evaluate (int j, unsigned char* gtype, node* gate, char* pattern_buffer, short* fanin, int** faninlist )
   {
    int nn;
    char out;
    char out_int;

         switch (gtype[j])
              { //switch begins
			 //INPUT
		          case 1:
		          gate[j].value=pattern_buffer[j];

		          break;

                              //NAND gate
			      case 7:
                                    out_int='1';
			             for (nn=0; nn<=(fanin[j]-1);nn++)
				     {
                                        if (gate[faninlist[j][nn]].value == '0')
					 {
					 gate[j].value='1';
					 return;
                                         }
                                         else if(gate[faninlist[j][nn]].value == '1')
                                         {
                                           if(out_int=='1')
                                              {
                                              out_int='1';
                                              }
                                           else if(out_int=='d')
                                              {
                                              out_int='d';
                                              }
                                           else if(out_int=='D')
                                              {
                                              out_int='D';
                                              }
                                           else if(out_int=='a')
                                              {
                                              out_int='a';
                                              }
                                           else if(out_int=='A')
                                              {
                                              out_int='A';
                                              }
                                           else if(out_int=='b')
                                              {
                                              out_int='b';
                                              }
                                           else if(out_int=='B')
                                              {
                                              out_int='B';
                                              }
                                           else if(out_int=='X')
                                              {
                                              out_int='X';
                                              }
                                         }
                                         else if(gate[faninlist[j][nn]].value == 'd')
                                         {
                                           if(out_int=='1')
                                              {
                                              out_int='d';
                                              }
                                           else if(out_int=='d')
                                              {
                                              out_int='d';
                                              }
                                           else if(out_int=='D')
                                              {
                                              gate[j].value='1';
                                              return;
                                              }
                                           else if(out_int=='a')
                                              {
                                              out_int='a';
                                              }
                                           else if(out_int=='A')
                                              {
                                              out_int='a';
                                              }
                                           else if(out_int=='b')
                                              {
                                              gate[j].value='1';
                                              return;
                                              }
                                           else if(out_int=='B')
                                              {
                                              out_int='d';
                                              }
                                           else if(out_int=='X')
                                              {
                                              out_int='a';
                                              }
                                         }
                                        else if(gate[faninlist[j][nn]].value == 'D')
                                         {
                                           if(out_int=='1')
                                              {
                                              out_int='D';
                                              }
                                           else if(out_int=='d')
                                              {
                                              gate[j].value='1';
                                              return;
                                              }
                                           else if(out_int=='D')
                                              {
                                              out_int='D';
                                              }
                                           else if(out_int=='a')
                                              {
                                              gate[j].value='1';
                                              return;
                                              }
                                           else if(out_int=='A')
                                              {
                                              out_int='D';
                                              }
                                           else if(out_int=='b')
                                              {
                                              out_int='b';
                                              }
                                           else if(out_int=='B')
                                              {
                                              out_int='b';
                                              }
                                           else if(out_int=='X')
                                              {
                                              out_int='b';
                                              }
                                         } 
                                        else if(gate[faninlist[j][nn]].value == 'a')
                                         {
                                           if(out_int=='1')
                                              {
                                              out_int='a';
                                              }
                                           else if(out_int=='d')
                                              {
                                              out_int='a';
                                              }
                                           else if(out_int=='D')
                                              {
                                              gate[j].value='1';
                                              return;
                                              }
                                           else if(out_int=='a')
                                              {
                                              out_int='a';
                                              }
                                           else if(out_int=='A')
                                              {
                                              out_int='a';
                                              }
                                           else if(out_int=='b')
                                              {
                                              gate[j].value='1';
                                              return;
                                              }
                                           else if(out_int=='B')
                                              {
                                              out_int='a';
                                              }
                                           else if(out_int=='X')
                                              {
                                              out_int='a';
                                              }
                                         }
                                        else if(gate[faninlist[j][nn]].value == 'A')
                                         {
                                           if(out_int=='1')
                                              {
                                              out_int='A';
                                              }
                                           else if(out_int=='d')
                                              {
                                              out_int='a';
                                              }
                                           else if(out_int=='D')
                                              {
                                              out_int='D';
                                              }
                                           else if(out_int=='a')
                                              {
                                              out_int='a';
                                              }
                                           else if(out_int=='A')
                                              {
                                              out_int='A';
                                              }
                                           else if(out_int=='b')
                                              {
                                              out_int='b';
                                              }
                                           else if(out_int=='B')
                                              {
                                              out_int='X';
                                              }
                                           else if(out_int=='X')
                                              {
                                              out_int='X';
                                              }
                                         }
                                        else if(gate[faninlist[j][nn]].value == 'b')
                                         {
                                           if(out_int=='1')
                                              {
                                              out_int='b';
                                              }
                                           else if(out_int=='d')
                                              {
                                              gate[j].value='1';
                                              return;
                                              }
                                           else if(out_int=='D')
                                              {
                                              out_int='b';
                                              }
                                           else if(out_int=='a')
                                              {
                                              gate[j].value='1';
                                              return;
                                              }
                                           else if(out_int=='A')
                                              {
                                              out_int='b';
                                              }
                                           else if(out_int=='b')
                                              {
                                              out_int='b';
                                              }
                                           else if(out_int=='B')
                                              {
                                              out_int='b';
                                              }
                                           else if(out_int=='X')
                                              {
                                              out_int='b';
                                              }
                                         }
                                        else if(gate[faninlist[j][nn]].value == 'B')
                                         {
                                           if(out_int=='1')
                                              {
                                              out_int='B';
                                              }
                                           else if(out_int=='d')
                                              {
                                              gate[j].value='1';
                                              return;
                                              }
                                           else if(out_int=='D')
                                              {
                                              out_int='b';
                                              }
                                           else if(out_int=='a')
                                              {
                                              out_int='a';
                                              }
                                           else if(out_int=='A')
                                              {
                                              out_int='X';
                                              }
                                           else if(out_int=='b')
                                              {
                                              out_int='b';
                                              }
                                           else if(out_int=='B')
                                              {
                                              out_int='B';
                                              }
                                           else if(out_int=='X')
                                              {
                                              out_int='X';
                                              }
                                         }
                                        else if(gate[faninlist[j][nn]].value == 'X')
                                         {
                                           if(out_int=='1')
                                              {
                                              out_int='X';
                                              }
                                           else if(out_int=='d')
                                              {
                                              out_int='a';
                                              }
                                           else if(out_int=='D')
                                              {
                                              out_int='b';
                                              }
                                           else if(out_int=='a')
                                              {
                                              out_int='a';
                                              }
                                           else if(out_int=='A')
                                              {
                                              out_int='X';
                                              }
                                           else if(out_int=='b')
                                              {
                                              out_int='b';
                                              }
                                           else if(out_int=='B')
                                              {
                                              out_int='X';
                                              }
                                           else if(out_int=='X')
                                              {
                                              out_int='X';
                                              }
                                         }
				     }

                                        switch (out_int)
                                            {
                                         case '0': out = '1';
                                                 break;
                                         case '1': out = '0';
                                                 break;
                                         case 'd': out = 'D';
                                                 break;
                                         case 'D': out = 'd';
                                                 break;
                                         case 'a': out = 'A';
                                                 break;
                                         case 'A': out = 'a';
                                                 break;
                                         case 'b': out = 'B';
                                                 break;
                                         case 'B': out = 'b';
                                                 break;
                                         case 'X': out = 'X';
                                                 break;
                                            }

                                         gate[j].value=out; 
					 break;

                              //AND gate
                              case 6:
                                    out_int='1';
			             for (nn=0; nn<=(fanin[j]-1);nn++)
				     {
                                        if (gate[faninlist[j][nn]].value == '0')
					 {
					 gate[j].value='0';
					 return;
                                         }
                                         else if(gate[faninlist[j][nn]].value == '1')
                                         {
                                           if(out_int=='1')
                                              {
                                              out_int='1';
                                              }
                                           else if(out_int=='d')
                                              {
                                              out_int='d';
                                              }
                                           else if(out_int=='D')
                                              {
                                              out_int='D';
                                              }
                                           else if(out_int=='a')
                                              {
                                              out_int='a';
                                              }
                                           else if(out_int=='A')
                                              {
                                              out_int='A';
                                              }
                                           else if(out_int=='b')
                                              {
                                              out_int='b';
                                              }
                                           else if(out_int=='B')
                                              {
                                              out_int='B';
                                              }
                                           else if(out_int=='X')
                                              {
                                              out_int='X';
                                              }
                                         }
                                         else if(gate[faninlist[j][nn]].value == 'd')
                                         {
                                           if(out_int=='1')
                                              {
                                              out_int='d';
                                              }
                                           else if(out_int=='d')
                                              {
                                              out_int='d';
                                              }
                                           else if(out_int=='D')
                                              {
                                              gate[j].value='0';
                                              return;
                                              }
                                           else if(out_int=='a')
                                              {
                                              out_int='a';
                                              }
                                           else if(out_int=='A')
                                              {
                                              out_int='a';
                                              }
                                           else if(out_int=='b')
                                              {
                                              gate[j].value='0';
                                              return;
                                              }
                                           else if(out_int=='B')
                                              {
                                              out_int='d';
                                              }
                                           else if(out_int=='X')
                                              {
                                              out_int='a';
                                              }
                                         }
                                        else if(gate[faninlist[j][nn]].value == 'D')
                                         {
                                           if(out_int=='1')
                                              {
                                              out_int='D';
                                              }
                                           else if(out_int=='d')
                                              {
                                              gate[j].value='0';
                                              return;
                                              }
                                           else if(out_int=='D')
                                              {
                                              out_int='D';
                                              }
                                           else if(out_int=='a')
                                              {
                                              gate[j].value='0';
                                              return;
                                              }
                                           else if(out_int=='A')
                                              {
                                              out_int='D';
                                              }
                                           else if(out_int=='b')
                                              {
                                              out_int='b';
                                              }
                                           else if(out_int=='B')
                                              {
                                              out_int='b';
                                              }
                                           else if(out_int=='X')
                                              {
                                              out_int='b';
                                              }
                                         } 
                                        else if(gate[faninlist[j][nn]].value == 'a')
                                         {
                                           if(out_int=='1')
                                              {
                                              out_int='a';
                                              }
                                           else if(out_int=='d')
                                              {
                                              out_int='a';
                                              }
                                           else if(out_int=='D')
                                              {
                                              gate[j].value='0';
                                              return;
                                              }
                                           else if(out_int=='a')
                                              {
                                              out_int='a';
                                              }
                                           else if(out_int=='A')
                                              {
                                              out_int='a';
                                              }
                                           else if(out_int=='b')
                                              {
                                              gate[j].value='0';
                                              return;
                                              }
                                           else if(out_int=='B')
                                              {
                                              out_int='a';
                                              }
                                           else if(out_int=='X')
                                              {
                                              out_int='a';
                                              }
                                         }
                                        else if(gate[faninlist[j][nn]].value == 'A')
                                         {
                                           if(out_int=='1')
                                              {
                                              out_int='A';
                                              }
                                           else if(out_int=='d')
                                              {
                                              out_int='a';
                                              }
                                           else if(out_int=='D')
                                              {
                                              out_int='D';
                                              }
                                           else if(out_int=='a')
                                              {
                                              out_int='a';
                                              }
                                           else if(out_int=='A')
                                              {
                                              out_int='A';
                                              }
                                           else if(out_int=='b')
                                              {
                                              out_int='b';
                                              }
                                           else if(out_int=='B')
                                              {
                                              out_int='X';
                                              }
                                           else if(out_int=='X')
                                              {
                                              out_int='X';
                                              }
                                         }
                                        else if(gate[faninlist[j][nn]].value == 'b')
                                         {
                                           if(out_int=='1')
                                              {
                                              out_int='b';
                                              }
                                           else if(out_int=='d')
                                              {
                                              gate[j].value='0';
                                              return;
                                              }
                                           else if(out_int=='D')
                                              {
                                              out_int='b';
                                              }
                                           else if(out_int=='a')
                                              {
                                              gate[j].value='0';
                                              return;
                                              }
                                           else if(out_int=='A')
                                              {
                                              out_int='b';
                                              }
                                           else if(out_int=='b')
                                              {
                                              out_int='b';
                                              }
                                           else if(out_int=='B')
                                              {
                                              out_int='b';
                                              }
                                           else if(out_int=='X')
                                              {
                                              out_int='b';
                                              }
                                         }
                                        else if(gate[faninlist[j][nn]].value == 'B')
                                         {
                                           if(out_int=='1')
                                              {
                                              out_int='B';
                                              }
                                           else if(out_int=='d')
                                              {
                                              gate[j].value='0';
                                              return;
                                              }
                                           else if(out_int=='D')
                                              {
                                              out_int='b';
                                              }
                                           else if(out_int=='a')
                                              {
                                              out_int='a';
                                              }
                                           else if(out_int=='A')
                                              {
                                              out_int='X';
                                              }
                                           else if(out_int=='b')
                                              {
                                              out_int='b';
                                              }
                                           else if(out_int=='B')
                                              {
                                              out_int='B';
                                              }
                                           else if(out_int=='X')
                                              {
                                              out_int='X';
                                              }
                                         }
                                        else if(gate[faninlist[j][nn]].value == 'X')
                                         {
                                           if(out_int=='1')
                                              {
                                              out_int='X';
                                              }
                                           else if(out_int=='d')
                                              {
                                              out_int='a';
                                              }
                                           else if(out_int=='D')
                                              {
                                              out_int='b';
                                              }
                                           else if(out_int=='a')
                                              {
                                              out_int='a';
                                              }
                                           else if(out_int=='A')
                                              {
                                              out_int='X';
                                              }
                                           else if(out_int=='b')
                                              {
                                              out_int='b';
                                              }
                                           else if(out_int=='B')
                                              {
                                              out_int='X';
                                              }
                                           else if(out_int=='X')
                                              {
                                              out_int='X';
                                              }
                                         }
				     }

                                        switch (out_int)
                                            {
                                         case '0': out = '0';
                                                 break;
                                         case '1': out = '1';
                                                 break;
                                         case 'd': out = 'd';
                                                 break;
                                         case 'D': out = 'D';
                                                 break;
                                         case 'a': out = 'a';
                                                 break;
                                         case 'A': out = 'A';
                                                 break;
                                         case 'b': out = 'b';
                                                 break;
                                         case 'B': out = 'B';
                                                 break;
                                         case 'X': out = 'X';
                                                 break;
                                            }

                                         gate[j].value=out; 
					 break;

                              //OR gate
                              case 8:
                                    out_int='0';
			             for (nn=0; nn<=(fanin[j]-1);nn++)
				     {
                                        if (gate[faninlist[j][nn]].value == '1')
					 {
					 gate[j].value='1';
					 return;
                                         }
                                         else if(gate[faninlist[j][nn]].value == '0')
                                         {
                                           if(out_int=='0')
                                              {
                                              out_int='0';
                                              }
                                           else if(out_int=='d')
                                              {
                                              out_int='d';
                                              }
                                           else if(out_int=='D')
                                              {
                                              out_int='D';
                                              }
                                           else if(out_int=='a')
                                              {
                                              out_int='a';
                                              }
                                           else if(out_int=='A')
                                              {
                                              out_int='A';
                                              }
                                           else if(out_int=='b')
                                              {
                                              out_int='b';
                                              }
                                           else if(out_int=='B')
                                              {
                                              out_int='B';
                                              }
                                           else if(out_int=='X')
                                              {
                                              out_int='X';
                                              }
                                         }
                                         else if(gate[faninlist[j][nn]].value == 'd')
                                         {
                                           if(out_int=='0')
                                              {
                                              out_int='d';
                                              }
                                           else if(out_int=='d')
                                              {
                                              out_int='d';
                                              }
                                           else if(out_int=='D')
                                              {
                                              gate[j].value='1';
                                              return;
                                              }
                                           else if(out_int=='a')
                                              {
                                              out_int='d';
                                              }
                                           else if(out_int=='A')
                                              {
                                              gate[j].value='1';
                                              return;
                                              }
                                           else if(out_int=='b')
                                              {
                                              out_int='B';
                                              }
                                           else if(out_int=='B')
                                              {
                                              out_int='B';
                                              }
                                           else if(out_int=='X')
                                              {
                                              out_int='B';
                                              }
                                         }
                                        else if(gate[faninlist[j][nn]].value == 'D')
                                         {
                                           if(out_int=='0')
                                              {
                                              out_int='D';
                                              }
                                           else if(out_int=='d')
                                              {
                                              gate[j].value='1';
                                              return;
                                              }
                                           else if(out_int=='D')
                                              {
                                              out_int='D';
                                              }
                                           else if(out_int=='a')
                                              {
                                              out_int='A';
                                              }
                                           else if(out_int=='A')
                                              {
                                              out_int='A';
                                              }
                                           else if(out_int=='b')
                                              {
                                              out_int='D';
                                              }
                                           else if(out_int=='B')
                                              {
                                              gate[j].value='1';
                                              return;
                                              }
                                           else if(out_int=='X')
                                              {
                                              out_int='A';
                                              }
                                         } 
                                        else if(gate[faninlist[j][nn]].value == 'a')
                                         {
                                           if(out_int=='0')
                                              {
                                              out_int='a';
                                              }
                                           else if(out_int=='d')
                                              {
                                              out_int='d';
                                              }
                                           else if(out_int=='D')
                                              {
                                              out_int='A';
                                              }
                                           else if(out_int=='a')
                                              {
                                              out_int='a';
                                              }
                                           else if(out_int=='A')
                                              {
                                              out_int='A';
                                              }
                                           else if(out_int=='b')
                                              {
                                              out_int='X';
                                              }
                                           else if(out_int=='B')
                                              {
                                              out_int='B';
                                              }
                                           else if(out_int=='X')
                                              {
                                              out_int='X';
                                              }
                                         }
                                        else if(gate[faninlist[j][nn]].value == 'A')
                                         {
                                           if(out_int=='0')
                                              {
                                              out_int='A';
                                              }
                                           else if(out_int=='d')
                                              {
                                              gate[j].value='1';
                                              return;
                                              }
                                           else if(out_int=='D')
                                              {
                                              out_int='A';
                                              }
                                           else if(out_int=='a')
                                              {
                                              out_int='A';
                                              }
                                           else if(out_int=='A')
                                              {
                                              out_int='A';
                                              }
                                           else if(out_int=='b')
                                              {
                                              out_int='A';
                                              }
                                           else if(out_int=='B')
                                              {
                                              gate[j].value='1';
                                              return;
                                              }
                                           else if(out_int=='X')
                                              {
                                              out_int='A';
                                              }
                                         }
                                        else if(gate[faninlist[j][nn]].value == 'b')
                                         {
                                           if(out_int=='0')
                                              {
                                              out_int='b';
                                              }
                                           else if(out_int=='d')
                                              {
                                              out_int='B'; 
                                              }
                                           else if(out_int=='D')
                                              {
                                              out_int='D';
                                              }
                                           else if(out_int=='a')
                                              {
                                              out_int='X';
                                              }
                                           else if(out_int=='A')
                                              {
                                              out_int='A';
                                              }
                                           else if(out_int=='b')
                                              {
                                              out_int='b';
                                              }
                                           else if(out_int=='B')
                                              {
                                              out_int='B';
                                              }
                                           else if(out_int=='X')
                                              {
                                              out_int='X';
                                              }
                                         }
                                        else if(gate[faninlist[j][nn]].value == 'B')
                                         {
                                           if(out_int=='0')
                                              {
                                              gate[j].value='1';
                                              return;
                                              }
                                           else if(out_int=='d')
                                              {
                                              out_int='B';
                                              }
                                           else if(out_int=='D')
                                              {
                                              gate[j].value='1';
                                              return;
                                              }
                                           else if(out_int=='a')
                                              {
                                              out_int='B';
                                              }
                                           else if(out_int=='A')
                                              {
                                              gate[j].value='1';
                                              return;
                                              }
                                           else if(out_int=='b')
                                              {
                                              out_int='B';
                                              }
                                           else if(out_int=='B')
                                              {
                                              out_int='B';
                                              }
                                           else if(out_int=='X')
                                              {
                                              out_int='B';
                                              }
                                         }
                                        else if(gate[faninlist[j][nn]].value == 'X')
                                         {
                                           if(out_int=='0')
                                              {
                                              out_int='X';
                                              }
                                           else if(out_int=='d')
                                              {
                                              out_int='B';
                                              }
                                           else if(out_int=='D')
                                              {
                                              out_int='A';
                                              }
                                           else if(out_int=='a')
                                              {
                                              out_int='X';
                                              }
                                           else if(out_int=='A')
                                              {
                                              out_int='A';
                                              }
                                           else if(out_int=='b')
                                              {
                                              out_int='X';
                                              }
                                           else if(out_int=='B')
                                              {
                                              out_int='B';
                                              }
                                           else if(out_int=='X')
                                              {
                                              out_int='X';
                                              }
                                         }
				     }

                                        switch (out_int)
                                            {
                                         case '0': out = '0';
                                                 break;
                                         case '1': out = '1';
                                                 break;
                                         case 'd': out = 'd';
                                                 break;
                                         case 'D': out = 'D';
                                                 break;
                                         case 'a': out = 'a';
                                                 break;
                                         case 'A': out = 'A';
                                                 break;
                                         case 'b': out = 'b';
                                                 break;
                                         case 'B': out = 'B';
                                                 break;
                                         case 'X': out = 'X';
                                                 break;
                                            }

                                         gate[j].value=out; 
					 break;

                              //NOR gate
                              case 9:

                                    out_int='0';
			             for (nn=0; nn<=(fanin[j]-1);nn++)
				     {
                                        if (gate[faninlist[j][nn]].value == '1')
					 {
					 gate[j].value='0';
					 return;
                                         }
                                         else if(gate[faninlist[j][nn]].value == '0')
                                         {
                                           if(out_int=='0')
                                              {
                                              out_int='0';
                                              }
                                           else if(out_int=='d')
                                              {
                                              out_int='d';
                                              }
                                           else if(out_int=='D')
                                              {
                                              out_int='D';
                                              }
                                           else if(out_int=='a')
                                              {
                                              out_int='a';
                                              }
                                           else if(out_int=='A')
                                              {
                                              out_int='A';
                                              }
                                           else if(out_int=='b')
                                              {
                                              out_int='b';
                                              }
                                           else if(out_int=='B')
                                              {
                                              out_int='B';
                                              }
                                           else if(out_int=='X')
                                              {
                                              out_int='X';
                                              }
                                         }
                                         else if(gate[faninlist[j][nn]].value == 'd')
                                         {
                                           if(out_int=='0')
                                              {
                                              out_int='d';
                                              }
                                           else if(out_int=='d')
                                              {
                                              out_int='d';
                                              }
                                           else if(out_int=='D')
                                              {
                                              gate[j].value='0';
                                              return;
                                              }
                                           else if(out_int=='a')
                                              {
                                              out_int='d';
                                              }
                                           else if(out_int=='A')
                                              {
                                              gate[j].value='0';
                                              return;
                                              }
                                           else if(out_int=='b')
                                              {
                                              out_int='B';
                                              }
                                           else if(out_int=='B')
                                              {
                                              out_int='B';
                                              }
                                           else if(out_int=='X')
                                              {
                                              out_int='B';
                                              }
                                         }
                                        else if(gate[faninlist[j][nn]].value == 'D')
                                         {
                                           if(out_int=='0')
                                              {
                                              out_int='D';
                                              }
                                           else if(out_int=='d')
                                              {
                                              gate[j].value='0';
                                              return;
                                              }
                                           else if(out_int=='D')
                                              {
                                              out_int='D';
                                              }
                                           else if(out_int=='a')
                                              {
                                              out_int='A';
                                              }
                                           else if(out_int=='A')
                                              {
                                              out_int='A';
                                              }
                                           else if(out_int=='b')
                                              {
                                              out_int='D';
                                              }
                                           else if(out_int=='B')
                                              {
                                              gate[j].value='0';
                                              return;
                                              }
                                           else if(out_int=='X')
                                              {
                                              out_int='A';
                                              }
                                         } 
                                        else if(gate[faninlist[j][nn]].value == 'a')
                                         {
                                           if(out_int=='0')
                                              {
                                              out_int='a';
                                              }
                                           else if(out_int=='d')
                                              {
                                              out_int='d';
                                              }
                                           else if(out_int=='D')
                                              {
                                              out_int='A';
                                              }
                                           else if(out_int=='a')
                                              {
                                              out_int='a';
                                              }
                                           else if(out_int=='A')
                                              {
                                              out_int='A';
                                              }
                                           else if(out_int=='b')
                                              {
                                              out_int='X';
                                              }
                                           else if(out_int=='B')
                                              {
                                              out_int='B';
                                              }
                                           else if(out_int=='X')
                                              {
                                              out_int='X';
                                              }
                                         }
                                        else if(gate[faninlist[j][nn]].value == 'A')
                                         {
                                           if(out_int=='0')
                                              {
                                              out_int='A';
                                              }
                                           else if(out_int=='d')
                                              {
                                              gate[j].value='0';
                                              return;
                                              }
                                           else if(out_int=='D')
                                              {
                                              out_int='A';
                                              }
                                           else if(out_int=='a')
                                              {
                                              out_int='A';
                                              }
                                           else if(out_int=='A')
                                              {
                                              out_int='A';
                                              }
                                           else if(out_int=='b')
                                              {
                                              out_int='A';
                                              }
                                           else if(out_int=='B')
                                              {
                                              gate[j].value='0';
                                              return;
                                              }
                                           else if(out_int=='X')
                                              {
                                              out_int='A';
                                              }
                                         }
                                        else if(gate[faninlist[j][nn]].value == 'b')
                                         {
                                           if(out_int=='0')
                                              {
                                              out_int='b';
                                              }
                                           else if(out_int=='d')
                                              {
                                              out_int='B'; 
                                              }
                                           else if(out_int=='D')
                                              {
                                              out_int='D';
                                              }
                                           else if(out_int=='a')
                                              {
                                              out_int='X';
                                              }
                                           else if(out_int=='A')
                                              {
                                              out_int='A';
                                              }
                                           else if(out_int=='b')
                                              {
                                              out_int='b';
                                              }
                                           else if(out_int=='B')
                                              {
                                              out_int='B';
                                              }
                                           else if(out_int=='X')
                                              {
                                              out_int='X';
                                              }
                                         }
                                        else if(gate[faninlist[j][nn]].value == 'B')
                                         {
                                           if(out_int=='0')
                                              {
                                              gate[j].value='0';
                                              return;
                                              }
                                           else if(out_int=='d')
                                              {
                                              out_int='B';
                                              }
                                           else if(out_int=='D')
                                              {
                                              gate[j].value='0';
                                              return;
                                              }
                                           else if(out_int=='a')
                                              {
                                              out_int='B';
                                              }
                                           else if(out_int=='A')
                                              {
                                              gate[j].value='0';
                                              return;
                                              }
                                           else if(out_int=='b')
                                              {
                                              out_int='B';
                                              }
                                           else if(out_int=='B')
                                              {
                                              out_int='B';
                                              }
                                           else if(out_int=='X')
                                              {
                                              out_int='B';
                                              }
                                         }
                                        else if(gate[faninlist[j][nn]].value == 'X')
                                         {
                                           if(out_int=='0')
                                              {
                                              out_int='X';
                                              }
                                           else if(out_int=='d')
                                              {
                                              out_int='B';
                                              }
                                           else if(out_int=='D')
                                              {
                                              out_int='A';
                                              }
                                           else if(out_int=='a')
                                              {
                                              out_int='X';
                                              }
                                           else if(out_int=='A')
                                              {
                                              out_int='A';
                                              }
                                           else if(out_int=='b')
                                              {
                                              out_int='X';
                                              }
                                           else if(out_int=='B')
                                              {
                                              out_int='B';
                                              }
                                           else if(out_int=='X')
                                              {
                                              out_int='X';
                                              }
                                         }
				     }

                                        switch (out_int)
                                            {
                                         case '0': out = '1';
                                                 break;
                                         case '1': out = '0';
                                                 break;
                                         case 'd': out = 'D';
                                                 break;
                                         case 'D': out = 'd';
                                                 break;
                                         case 'a': out = 'A';
                                                 break;
                                         case 'A': out = 'a';
                                                 break;
                                         case 'b': out = 'B';
                                                 break;
                                         case 'B': out = 'b';
                                                 break;
                                         case 'X': out = 'X';
                                                 break;
                                            }

                                         gate[j].value=out; 
					 break;

                              //XOR gate
			            case 3:
                                      out_int='0';
                                      for (nn=0; nn<=(fanin[j]-1);nn++)
				         {
                                        if (gate[faninlist[j][nn]].value == 'X')
					 {
					 gate[j].value='X';
					 return;
                                         }
                                         else if (gate[faninlist[j][nn]].value == '1')
                                          {
                                             if(out_int=='1')
                                             out_int='0';
                                             else if(out_int=='0')
                                             out_int='1';
                                             else if(out_int=='d')
                                             out_int='D';
                                             else if(out_int=='D')
                                             out_int='d';
                                             else if(out_int=='a')
                                             out_int='A';
                                             else if(out_int=='A')
                                             out_int='a';
                                             else if(out_int=='b')
                                             out_int='B';
                                             else if(out_int=='B')
                                             out_int='b';
                                          }
                                         else if (gate[faninlist[j][nn]].value == '0')
                                          {
                                             if(out_int=='1')
                                             out_int='1';
                                             else if(out_int=='0')
                                             out_int='0';
                                             else if(out_int=='d')
                                             out_int='d';
                                             else if(out_int=='D')
                                             out_int='D';
                                             else if(out_int=='a')
                                             out_int='a';
                                             else if(out_int=='A')
                                             out_int='A';
                                             else if(out_int=='b')
                                             out_int='b';
                                             else if(out_int=='B')
                                             out_int='B';

                                          }
                                         else if (gate[faninlist[j][nn]].value == 'd')
                                          {
                                             if(out_int=='1')
                                             out_int='D';
                                             else if(out_int=='0')
                                             out_int='d';
                                             else if(out_int=='d')
                                             out_int='0';
                                             else if(out_int=='D')
                                             out_int='1';
                                             else if(out_int=='a')
                                             out_int='a';
                                             else if(out_int=='A')
                                             out_int='A';
                                             else if(out_int=='b')
                                             out_int='B';
                                             else if(out_int=='B')
                                             out_int='b';
                                          }
                                         else if (gate[faninlist[j][nn]].value == 'D')
                                          {
                                             if(out_int=='1')
                                             out_int='d';
                                             else if(out_int=='0')
                                             out_int='D';
                                             else if(out_int=='d')
                                             out_int='1';
                                             else if(out_int=='D')
                                             out_int='0';
                                             else if(out_int=='a')
                                             out_int='A';
                                             else if(out_int=='A')
                                             out_int='a';
                                             else if(out_int=='b')
                                             out_int='b';
                                             else if(out_int=='B')
                                             out_int='B';
                                          }
                                         else if (gate[faninlist[j][nn]].value == 'a')
                                          {
                                             if(out_int=='1')
                                             out_int='A';
                                             else if(out_int=='0')
                                             out_int='a';
                                             else if(out_int=='d')
                                             out_int='a';
                                             else if(out_int=='D')
                                             out_int='A';
                                             else if(out_int=='a')
                                             out_int='a';
                                             else if(out_int=='A')
                                             out_int='A';
                                             else if(out_int=='b')
                                              {
					     gate[j].value='X';
					     return;
                                              }
                                             else if(out_int=='B')
                                              {
					     gate[j].value='X';
					     return;
                                              }
                                          }
                                         else if (gate[faninlist[j][nn]].value == 'A')
                                          {
                                             if(out_int=='1')
                                             out_int='a';
                                             else if(out_int=='0')
                                             out_int='A';
                                             else if(out_int=='d')
                                             out_int='A';
                                             else if(out_int=='D')
                                             out_int='a';
                                             else if(out_int=='a')
                                             out_int='A';
                                             else if(out_int=='A')
                                             out_int='a';
                                             else if(out_int=='b')
                                              {
					     gate[j].value='X';
					     return;
                                              }
                                             else if(out_int=='B')
                                              {
					     gate[j].value='X';
					     return;
                                              }
                                          }
                                         else if (gate[faninlist[j][nn]].value == 'b')
                                          {
                                             if(out_int=='1')
                                             out_int='B';
                                             else if(out_int=='0')
                                             out_int='b';
                                             else if(out_int=='d')
                                             out_int='B';
                                             else if(out_int=='D')
                                             out_int='b';
                                             else if(out_int=='a')
                                              {
					     gate[j].value='X';
					     return;
                                              }
                                             else if(out_int=='A')
                                              {
					     gate[j].value='X';
					     return;
                                              }
                                             else if(out_int=='b')
                                             out_int='b';
                                             else if(out_int=='B')
                                             out_int='B';
                                          }
                                         else if (gate[faninlist[j][nn]].value == 'B')
                                          {
                                             if(out_int=='1')
                                             out_int='b';
                                             else if(out_int=='0')
                                             out_int='B';
                                             else if(out_int=='d')
                                             out_int='b';
                                             else if(out_int=='D')
                                             out_int='B';
                                             else if(out_int=='a')
                                              {
					     gate[j].value='X';
					     return;
                                              }
                                             else if(out_int=='A')
                                              {
					     gate[j].value='X';
					     return;
                                              }
                                             else if(out_int=='b')
                                             out_int='B';
                                             else if(out_int=='B')
                                             out_int='b';
                                          }
                                       }
                                        switch (out_int)
                                            {
                                         case '0': out = '0';
                                                 break;
                                         case '1': out = '1';
                                                 break;
                                         case 'd': out = 'd';
                                                 break;
                                         case 'D': out = 'D';
                                                 break;
                                         case 'a': out = 'a';
                                                 break;
                                         case 'A': out = 'A';
                                                 break;
                                         case 'b': out = 'b';
                                                 break;
                                         case 'B': out = 'B';
                                                 break;
                                            }
                                         gate[j].value=out; 
					 break; 

                             //XNOR gate
			            case 4:
                                      out_int='0';
                                      for (nn=0; nn<=(fanin[j]-1);nn++)
				         {
                                        if (gate[faninlist[j][nn]].value == 'X')
					 {
					 gate[j].value='X';
					 return;
                                         }
                                         else if (gate[faninlist[j][nn]].value == '1')
                                          {
                                             if(out_int=='1')
                                             out_int='0';
                                             else if(out_int=='0')
                                             out_int='1';
                                             else if(out_int=='d')
                                             out_int='D';
                                             else if(out_int=='D')
                                             out_int='d';
                                             else if(out_int=='a')
                                             out_int='A';
                                             else if(out_int=='A')
                                             out_int='a';
                                             else if(out_int=='b')
                                             out_int='B';
                                             else if(out_int=='B')
                                             out_int='b';
                                          }
                                         else if (gate[faninlist[j][nn]].value == '0')
                                          {
                                             if(out_int=='1')
                                             out_int='1';
                                             else if(out_int=='0')
                                             out_int='0';
                                             else if(out_int=='d')
                                             out_int='d';
                                             else if(out_int=='D')
                                             out_int='D';
                                             else if(out_int=='a')
                                             out_int='a';
                                             else if(out_int=='A')
                                             out_int='A';
                                             else if(out_int=='b')
                                             out_int='b';
                                             else if(out_int=='B')
                                             out_int='B';

                                          }
                                         else if (gate[faninlist[j][nn]].value == 'd')
                                          {
                                             if(out_int=='1')
                                             out_int='D';
                                             else if(out_int=='0')
                                             out_int='d';
                                             else if(out_int=='d')
                                             out_int='0';
                                             else if(out_int=='D')
                                             out_int='1';
                                             else if(out_int=='a')
                                             out_int='a';
                                             else if(out_int=='A')
                                             out_int='A';
                                             else if(out_int=='b')
                                             out_int='B';
                                             else if(out_int=='B')
                                             out_int='b';
                                          }
                                         else if (gate[faninlist[j][nn]].value == 'D')
                                          {
                                             if(out_int=='1')
                                             out_int='d';
                                             else if(out_int=='0')
                                             out_int='D';
                                             else if(out_int=='d')
                                             out_int='1';
                                             else if(out_int=='D')
                                             out_int='0';
                                             else if(out_int=='a')
                                             out_int='A';
                                             else if(out_int=='A')
                                             out_int='a';
                                             else if(out_int=='b')
                                             out_int='b';
                                             else if(out_int=='B')
                                             out_int='B';
                                          }
                                         else if (gate[faninlist[j][nn]].value == 'a')
                                          {
                                             if(out_int=='1')
                                             out_int='A';
                                             else if(out_int=='0')
                                             out_int='a';
                                             else if(out_int=='d')
                                             out_int='a';
                                             else if(out_int=='D')
                                             out_int='A';
                                             else if(out_int=='a')
                                             out_int='a';
                                             else if(out_int=='A')
                                             out_int='A';
                                             else if(out_int=='b')
                                              {
					     gate[j].value='X';
					     return;
                                              }
                                             else if(out_int=='B')
                                              {
					     gate[j].value='X';
					     return;
                                              }
                                          }
                                         else if (gate[faninlist[j][nn]].value == 'A')
                                          {
                                             if(out_int=='1')
                                             out_int='a';
                                             else if(out_int=='0')
                                             out_int='A';
                                             else if(out_int=='d')
                                             out_int='A';
                                             else if(out_int=='D')
                                             out_int='a';
                                             else if(out_int=='a')
                                             out_int='A';
                                             else if(out_int=='A')
                                             out_int='a';
                                             else if(out_int=='b')
                                              {
					     gate[j].value='X';
					     return;
                                              }
                                             else if(out_int=='B')
                                              {
					     gate[j].value='X';
					     return;
                                              }
                                          }
                                         else if (gate[faninlist[j][nn]].value == 'b')
                                          {
                                             if(out_int=='1')
                                             out_int='B';
                                             else if(out_int=='0')
                                             out_int='b';
                                             else if(out_int=='d')
                                             out_int='B';
                                             else if(out_int=='D')
                                             out_int='b';
                                             else if(out_int=='a')
                                              {
					     gate[j].value='X';
					     return;
                                              }
                                             else if(out_int=='A')
                                              {
					     gate[j].value='X';
					     return;
                                              }
                                             else if(out_int=='b')
                                             out_int='b';
                                             else if(out_int=='B')
                                             out_int='B';
                                          }
                                         else if (gate[faninlist[j][nn]].value == 'B')
                                          {
                                             if(out_int=='1')
                                             out_int='b';
                                             else if(out_int=='0')
                                             out_int='B';
                                             else if(out_int=='d')
                                             out_int='b';
                                             else if(out_int=='D')
                                             out_int='B';
                                             else if(out_int=='a')
                                              {
					     gate[j].value='X';
					     return;
                                              }
                                             else if(out_int=='A')
                                              {
					     gate[j].value='X';
					     return;
                                              }
                                             else if(out_int=='b')
                                             out_int='B';
                                             else if(out_int=='B')
                                             out_int='b';
                                          }
                                        }
                                        switch (out_int)
                                            {
                                         case '0': out = '1';
                                                 break;
                                         case '1': out = '0';
                                                 break;
                                         case 'd': out = 'D';
                                                 break;
                                         case 'D': out = 'd';
                                                 break;
                                         case 'a': out = 'A';
                                                 break;
                                         case 'A': out = 'a';
                                                 break;
                                         case 'b': out = 'B';
                                                 break;
                                         case 'B': out = 'b';
                                                 break;
                                            }
                                         gate[j].value=out; 
					 break;

			      //NOT gate
                              case 10:
				        if ( gate[faninlist[j][0]].value == '0')
					        gate[j].value='1';
					else if (gate[faninlist[j][0]].value == '1')
					       gate[j].value='0';
					else if (gate[faninlist[j][0]].value == 'd')
					       gate[j].value='D';
					else if (gate[faninlist[j][0]].value == 'D')
					       gate[j].value='d';
					else if (gate[faninlist[j][0]].value == 'a')
					       gate[j].value='A';
					else if (gate[faninlist[j][0]].value == 'A')
					       gate[j].value='a';
					else if (gate[faninlist[j][0]].value == 'b')
					       gate[j].value='B';
					else if (gate[faninlist[j][0]].value == 'B')
					       gate[j].value='b';
					else if (gate[faninlist[j][0]].value == 'X')
					       gate[j].value='X';

					break;

                             //BUF gate
			      case 11:

				  	gate[j].value=gate[faninlist[j][0]].value;
					break;

                              //TIE_1
                              case 12:
					gate[j].value='1';
					break;

                              //TIE_0
			      case 13:
				        gate[j].value = '0';
				        break;

                              //OUTPUT
			      case 2:
				        gate[j].value=gate[faninlist[j][0]].value;
				        break;

			      //D_FF
			      case 5:
                                        gate[j].value=gate[faninlist[j][0]].value;
				        break;

			   } //switch ends


		} //function ends

////////////////////////////////////////////////////////////////////////
// main starts here
////////////////////////////////////////////////////////////////////////
main(int argc, char *argv[])
{
    char cktName[20];
    circuit *ckt;
    int vecWidth, poNum;

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <ckt>\n", argv[0]);
        exit(-1);
    }

    strcpy(cktName, argv[1]);

    // read in the circuit and build data structure
    ckt = new circuit(cktName);
    // NOW ADD YOUR FUNCTION HERE
}

