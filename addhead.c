/*
/*
 *  ADDHEAD utility (c) Copyright, Kevin Thacker 2001-2005
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/* This utility can be used to add a AMSDOS header to a file. The file will
 * be treated by the Amstrad operating system as binary. The resulting file
 * can be injected into a disk image.
 *
 * This utility is ideal as part of a cross-development tool.
*/
#include "opth.h"
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
typedef int BOOL;
#ifndef TRUE
#define TRUE (1==1)
#endif

#ifndef FALSE
#define FALSE (1==0)
#endif

/* I am using a enum, so that I can poke data into structures without
worrying how the compiler has aligned it */
enum
{
	CPC_DISC_HEADER_FILENAME_BYTE0 = 0,
	CPC_DISC_HEADER_FILENAME_BYTE1,
	CPC_DISC_HEADER_FILENAME_BYTE2,
	CPC_DISC_HEADER_FILENAME_BYTE3,
	CPC_DISC_HEADER_FILENAME_BYTE4,
	CPC_DISC_HEADER_FILENAME_BYTE5,
	CPC_DISC_HEADER_FILENAME_BYTE6,
	CPC_DISC_HEADER_FILENAME_BYTE7,
	CPC_DISC_HEADER_FILENAME_BYTE8,
	CPC_DISC_HEADER_FILENAME_EXTENSION0,
	CPC_DISC_HEADER_FILENAME_EXTENSION1,
	CPC_DISC_HEADER_FILENAME_EXTENSION2,
	CPC_DISC_HEADER_FILE_TYPE = 18,
	CPC_DISC_HEADER_LENGTH_LOW,
	CPC_DISC_HEADER_LENGTH_HIGH,
	CPC_DISC_HEADER_LOCATION_LOW,
	CPC_DISC_HEADER_LOCATION_HIGH,
	CPC_DISC_HEADER_FIRST_BLOCK_FLAG,
	CPC_DISC_HEADER_LOGICAL_LENGTH_LOW,
	CPC_DISC_HEADER_LOGICAL_LENGTH_HIGH,
	CPC_DISC_HEADER_EXECUTION_ADDRESS_LOW,
	CPC_DISC_HEADER_EXECUTION_ADDRESS_HIGH,
	CPC_DISC_HEADER_DATA_LENGTH_LOW = 64,
	CPC_DISC_HEADER_DATA_LENGTH_MID,
	CPC_DISC_HEADER_DATA_LENGTH_HIGH,
	CPC_DISC_HEADER_CHECKSUM_LOW,
	CPC_DISC_HEADER_CHECKSUM_HIGH
} CPC_DISC_HEADER_ENUM;

static int AMSDOSHeader_AllZeros(const char *pData)
{
  unsigned char ordata = 0;
  int i;
    	for (i=0; i<69; i++)
	{
    unsigned char data = pData[i];
    ordata|=data;
	}

    if (ordata)
        return 0;
    return 1;
}

/*--------------------*/
/* calculate checksum */
static unsigned short AMSDOSHeader_CalculateChecksum(const char *pData)
{
	unsigned short CalculatedChecksum = 0;
	unsigned long i;

	/* generate checksum */
	for (i=0; i<66; i++)
	{
    unsigned char data = pData[i];
		CalculatedChecksum+=(unsigned short)(data&0x0ff);
	}
	return CalculatedChecksum;
}

/*------------------------------------------------*/
/* Detect if there is a AMSDOS header on the file */
static int AMSDOSHeader_Checksum(const char *pData)
{
  if (AMSDOSHeader_AllZeros(pData))
  {
    return 0;
  }
else
{  
  
	unsigned short StoredChecksum;
	unsigned short CalculatedChecksum;

	CalculatedChecksum = AMSDOSHeader_CalculateChecksum(pData);

	/* get the stored checksum */
	StoredChecksum = (unsigned short)((pData[67]&0x0ff)|((pData[68]&0x0ff)<<8));

	return (CalculatedChecksum==StoredChecksum);
}
}


enum
{
	OPERATION_UNKNOWN = 0,
	OPERATION_ADD_HEADER,
	OPERATION_REMOVE_HEADER,
	OPERATION_CHECK_LENGTH
};

/* buffer to hold generated header data */
static char HeaderBuffer[128];
static int nExecutionAddress = 0;
static int bExecutionAddressSet = FALSE;
static int nStartAddress = 0;
static BOOL bStartAddressSet=FALSE;
static int nFileType = 2;
static BOOL bFileTypeSet=FALSE;
static BOOL bForce = FALSE;
#define DEFAULT_START 0x01000
#define DEFAULT_TYPE 2
static int nOp= OPERATION_ADD_HEADER;

//static int bAddHeader;
static int bUseHeaderLength;
static int nMaxAddress;
/*------------------------------------------------*/
static void AMSDOSHeader_Initialise(char *pData, unsigned long Length)
{
	unsigned short CalculatedChecksum;

	int nType = DEFAULT_TYPE;
	int nStart = DEFAULT_START;
	int nExec = DEFAULT_START;

	memset(pData, 0, 128);

	// if type is set, use it, else use default
	if (bFileTypeSet)
	{
		nType = nFileType;
	}
	printf("File type used: %d\n", nType);

	// if start is set, use it, else use default.
	if (bStartAddressSet)
	{
		nStart = nStartAddress;
	}
	printf("Start used: %d (&%04x)\n", nStart,nStart);

	// if execution is set, use it, else use default.
	if (bExecutionAddressSet)
	{
		nExec = nExecutionAddress;
	}
	printf("Execution used: %d (&%04x)\n", nExec,nExec);


	pData[CPC_DISC_HEADER_FILE_TYPE] = nType;

	pData[CPC_DISC_HEADER_FIRST_BLOCK_FLAG] = (char)(0x0ff);

	pData[CPC_DISC_HEADER_DATA_LENGTH_LOW] = (char)(Length&0x0ff);
	pData[CPC_DISC_HEADER_DATA_LENGTH_MID] = (char)((Length>>8)&0x0ff);
	pData[CPC_DISC_HEADER_DATA_LENGTH_HIGH] = (char)((Length>>16)&0x0ff);

	printf("Length used: %d (&%0x)\n", (int)Length, (int)Length);
	
	pData[CPC_DISC_HEADER_LOCATION_LOW] = (char)(nStart&0x0ff);
	pData[CPC_DISC_HEADER_LOCATION_HIGH] = (char)((nStart>>8)&0x0ff);

/*	pData[CPC_DISC_HEADER_LENGTH_LOW] = pData[CPC_DISC_HEADER_DATA_LENGTH_LOW];
	pData[CPC_DISC_HEADER_LENGTH_HIGH] = pData[CPC_DISC_HEADER_DATA_LENGTH_MID];
*/
	pData[CPC_DISC_HEADER_LOGICAL_LENGTH_LOW] = pData[CPC_DISC_HEADER_DATA_LENGTH_LOW];
	pData[CPC_DISC_HEADER_LOGICAL_LENGTH_HIGH] = pData[CPC_DISC_HEADER_DATA_LENGTH_MID];

	pData[CPC_DISC_HEADER_EXECUTION_ADDRESS_LOW] = (nExec&0x0ff);
	pData[CPC_DISC_HEADER_EXECUTION_ADDRESS_HIGH] = (nExec>>8)&0x0ff;

	CalculatedChecksum = AMSDOSHeader_CalculateChecksum(pData);

	pData[CPC_DISC_HEADER_CHECKSUM_LOW] = (char)(CalculatedChecksum&0x0ff);
	pData[CPC_DISC_HEADER_CHECKSUM_HIGH] = (char)((CalculatedChecksum>>8)&0x0ff);
}


int ReadNumberParameter(const char *param)
{
    int Length = strlen(param);
    BOOL bIsHex = FALSE;
    int Offset = 0;
    unsigned long Value = 0;
    char ch;

    if (Length==0)
        return 0;

    /* check for common prefixs for hex numbers */
    if ((Length>1) && ((param[0]=='&') || (param[0]=='$')))
    {
        Offset = 1;
        bIsHex = TRUE;
    }
    else if ((Length>2) && (param[0]=='0') && ((param[1]=='x') || (param[1]=='X')))
    {
        Offset = 2;
        bIsHex = TRUE;
    }

    if (!bIsHex)
    {
        return atoi(param);
    }

    ch = param[Offset];
    while (ch!='\0')
    {
        Value = Value<<4;
        if ((ch>='0') && (ch<='9'))
        {
            Value = Value | (ch-'0');
        }
        else if ((ch>='a') && (ch<='f'))
        {
            Value = Value | ((ch-'a')+10);
        }
        else if ((ch>='A') && (ch<='F'))
        {
            Value = Value | ((ch-'A')+10);
        }
        Offset++;
        ch = param[Offset];
    }

    return Value;
}

static int NumFiles = 0;
static const char *Filenames[2];

int	NonOptionHandler(const char *pOption)
{
	if (NumFiles<2)
	{
		Filenames[NumFiles] = pOption;
		NumFiles++;
	}

	return OPTION_OK;
}

int AddHeaderOption(ARGUMENT_DATA *pData)
{
	nOp = OPERATION_ADD_HEADER;

	return OPTION_OK;
}


int ForceOption(ARGUMENT_DATA *pData)
{
	bForce = TRUE;
  
	return OPTION_OK;
}


int RemoveHeaderOption(ARGUMENT_DATA *pData)
{
	nOp = OPERATION_REMOVE_HEADER;

	return OPTION_OK;
}

int UseHeaderLengthOption(ARGUMENT_DATA *pData)
{
	bUseHeaderLength = TRUE;

	return OPTION_OK;
}

int SetFileTypeOption(ARGUMENT_DATA *pData)
{
	const char *pAddress = ArgumentList_GetNext(pData);

	if (pAddress==NULL)
		return OPTION_MISSING_PARAMETER;

	if (strcmp(pAddress,"binary")==0)
	{
		printf("User defined type as \"binary\"\n");
		bFileTypeSet = TRUE;
		nFileType=2;
	}
	else if (strcmp(pAddress,"basic")==0)
	{
		printf("User defined type as \"basic\"\n");
 	 	nFileType=0;
		bFileTypeSet = TRUE;
	}

	return OPTION_OK;
}

int SetExecutionAddressOption(ARGUMENT_DATA *pData)
{
	const char *pAddress = ArgumentList_GetNext(pData);

	if (pAddress==NULL)
		return OPTION_MISSING_PARAMETER;

	nExecutionAddress = ReadNumberParameter(pAddress);
	printf("User defined execution address as %d (&%04x)\n", nExecutionAddress, nExecutionAddress);

	bExecutionAddressSet = TRUE;

	return OPTION_OK;
}

int SetStartAddressOption(ARGUMENT_DATA *pData)
{
	const char *pAddress = ArgumentList_GetNext(pData);

	if (pAddress==NULL)
		return OPTION_MISSING_PARAMETER;

	nStartAddress = ReadNumberParameter(pAddress);
	printf("User defined start address as %d (&%04x)\n", nStartAddress, nStartAddress);

	bStartAddressSet = TRUE;

	return OPTION_OK;
}


int CheckLengthOption(ARGUMENT_DATA *pData)
{
	const char *pAddress = ArgumentList_GetNext(pData);

	if (pAddress==NULL)
		return OPTION_MISSING_PARAMETER;

	nMaxAddress = ReadNumberParameter(pAddress);
	printf("User set Max address as %d (&%04x\"\n", nMaxAddress, nMaxAddress);

	nOp = OPERATION_CHECK_LENGTH;

	return OPTION_OK;
}


void	OutputDetails(void)
{
	fprintf(stdout,"ADDHEAD\n");
	fprintf(stdout,"\n");
	fprintf(stdout,"(c) Kevin Thacker 2001-2010\n");
	fprintf(stdout,"A utility to add/remove a AMSDOS header to a file.\n");
	fprintf(stdout,"\n");
	fprintf(stdout,"Usage: ADDHEAD <switches> <input filename> <output filename>\n");
	fprintf(stdout,"\n");
	fprintf(stdout,"-a     - add a header to file\n");
	fprintf(stdout,"-r     - remove a existing header\n");
	fprintf(stdout,"-f     - force operation\n");
	fprintf(stdout,"-c <number>		- check length against max\n");

	fprintf(stdout,"-h - use header length to write\n");

	fprintf(stdout,"-t <number>    - set file type (\"binary\" (default), \"basic\" (default \"binary\")");
	fprintf(stdout,"-x <number>    - set execution address (default=start)\n");
	fprintf(stdout,"-s <number> - set start address (default 0x01000)\n");
	fprintf(stdout,"-?   - show this information\n");
	fprintf(stdout,"\n");
}


int OutputDetailsOption(ARGUMENT_DATA *pData)
{
	OutputDetails();
	return OPTION_OK;
}

OPTION OptionTable[]=
{
	{"a",AddHeaderOption},
	{"r",RemoveHeaderOption},
	{"c",CheckLengthOption},
	{"f",ForceOption},
	{"?",OutputDetailsOption},

	{"h",UseHeaderLengthOption},

	{"s",SetStartAddressOption},
	{"t",SetFileTypeOption},
	{"x",SetExecutionAddressOption},
	{NULL, NULL},
};



int	main(int argc, char *argv[])
{
	int nReturnCode= 0;

	bUseHeaderLength = FALSE;
	bExecutionAddressSet = FALSE;
	bStartAddressSet = FALSE;
	bFileTypeSet = FALSE;

	NumFiles = 0;

	if (ArgumentList_Execute(argc, argv, OptionTable, printf, NonOptionHandler)==OPTION_OK)
	{
		if (NumFiles==0)
		{
			OutputDetails();
			nReturnCode = 0;
		}
		else
		if ((nOp!=OPERATION_CHECK_LENGTH) && (NumFiles!=2))
		{
			fprintf(stderr,"Expected 2 files to be specified!\n");
			
			nReturnCode = 1;
		}
		else if ((nOp==OPERATION_CHECK_LENGTH) && (NumFiles!=1))
		{
			fprintf(stderr,"Expected 1 file to be specified!\n");
		}
		else
		{

			FILE *fh_in, *fh_out;
			unsigned long filesize;

			/* open input binary file */
			fh_in = fopen(Filenames[0],"rb");

			if (fh_in==NULL)
			{
				printf("Failed to open input file\n");
				nReturnCode = 1;
			}
			else
			{
				unsigned char *file_data;

				/* seek to end of file */
				fseek(fh_in, 0, SEEK_END);

				/* report position in file */
				filesize = ftell(fh_in);

				/* seek to beginning of file */
				fseek(fh_in, 0,SEEK_SET);

				file_data = (unsigned char *)malloc(filesize);

				if (file_data!=NULL)
				{
					fread(file_data, 1, filesize, fh_in);
				}
				else
				{
					printf("Failed to allocate memory for input file.\n");
					nReturnCode = 1;
				}

				fclose(fh_in);

				if (file_data!=NULL)
				{
					BOOL bSourceHasHeader = FALSE;
					if (AMSDOSHeader_Checksum((const char *)file_data))
					{
						printf("AMSDOS header detected.\n");
						bSourceHasHeader = TRUE;
					}

					switch (nOp)
					{
						case OPERATION_REMOVE_HEADER:
						{
							/* removing a existing header */
							printf("Removing header.\n");

							/* file must have at least the size of the header */
							if (filesize>=128)
							{
								if (bSourceHasHeader)
								{
								    int WriteLength = filesize-128;
								    int HeaderLength = (file_data[CPC_DISC_HEADER_DATA_LENGTH_LOW]&0x0ff) |
	                                                    ((file_data[CPC_DISC_HEADER_DATA_LENGTH_MID]&0x0ff)<<8) |
	                                                    ((file_data[CPC_DISC_HEADER_DATA_LENGTH_HIGH]&0x0ff)<<16);

	                                if (bUseHeaderLength)
	                                {
	                                    printf("Using header length.\n");
	                                    WriteLength = HeaderLength;
	                                }

	                                printf("Length of data to write: %d\n", WriteLength);

									/* now open output file */
									fh_out = fopen(Filenames[1],"wb");

									if (fh_out==NULL)
									{
										printf("Failed to open output file.\n");
										nReturnCode = 1;
									}
									else
									{
										/* copy remaining data to output */
										fwrite(file_data+128, 1, WriteLength, fh_out);

										fclose(fh_out);

										printf("Output file written.\n");
									}

								}
								else
								{
									printf("The input file does not have a AMSDOS header.\n");
									if (!bForce)
                  {
                      nReturnCode = 1;
                  }
                }
							}
							else
							{
								printf("The input file is too small to have a AMSDOS header.\n");
								nReturnCode = 1;
							}
						}
						break;

						case OPERATION_CHECK_LENGTH:
						{
							int nStart;
							int nLength;
							int Diff;

							printf("Checking file length.\n");

							if (bSourceHasHeader)
							{
								if (bUseHeaderLength)
								{
									printf("Using header length.\n");

									// length from header
								  	nLength = (file_data[CPC_DISC_HEADER_DATA_LENGTH_LOW]&0x0ff) |
	                                                    ((file_data[CPC_DISC_HEADER_DATA_LENGTH_MID]&0x0ff)<<8) |
	                                                    ((file_data[CPC_DISC_HEADER_DATA_LENGTH_HIGH]&0x0ff)<<16);
								}
								else
								{
									// length - header length
									nLength = filesize-128;
								}

								nStart = (file_data[CPC_DISC_HEADER_LOCATION_LOW]&0x0ff) |
	                                     ((file_data[CPC_DISC_HEADER_LOCATION_HIGH]&0x0ff)<<8);

							}
							else
							{
								nLength = filesize;

								if (!bStartAddressSet)
								{
									printf("Start address not set and file doesn't have header!\n");
									nReturnCode = 1;
									break;
								}
								nStart = DEFAULT_START;
							}


							// override
							if (bStartAddressSet)
							{
								nStart = nStartAddress;
							}

                            printf("Start of data to check: %d\n", nStart);
                            printf("Length of data to check: %d\n", nLength);
							printf("Max address: %d\n",nMaxAddress);

                            Diff = nMaxAddress-(nStart+nLength);

                            if (Diff>=0)
                            {
	                            printf("File is (%d bytes) less than maximum.\n", Diff);
                            }
                            else
                            if (Diff<0)
                            {
	                            /* file is too big */
	                            printf("File is (%d bytes) too BIG!\n",-Diff);
	                            nReturnCode = 2;
                            }
						}
						break;


						case OPERATION_ADD_HEADER:
						{
              BOOL bHasHeader = FALSE;
							/* adding a header */
							printf("Adding header.\n");

							/* calculate checksum and compare against stored checksum */
							/* if valid, then file is assumed to have a header */
							if (AMSDOSHeader_Checksum((const char *)file_data))
							{
                bHasHeader = TRUE;
              }
              
              if (bHasHeader && !bForce)
              {
                /* force is off, so report error and quit */
								printf("File seems to have an Amsdos header already!\n");
								nReturnCode = 1;
							}
							else
							{

								/* now open output file */
								fh_out = fopen(Filenames[1],"wb");

								if (fh_out==NULL)
								{
									printf("Failed to open output file.\n");
									nReturnCode = 1;
								}
								else
								{
                  unsigned char *file_start = file_data;
                  
                  /* force was switched on, or no header */
                  if (bHasHeader)
                  {
                      /* skip existing header */
                      filesize -=128;
                      file_start +=128;
                  }
									AMSDOSHeader_Initialise(HeaderBuffer, filesize);

									/* write header to output file */
									fwrite(HeaderBuffer, 1, 128, fh_out);

									/* copy remaining data to output */
                  if (filesize!=0)
                  {
                    fwrite(file_start, 1, filesize, fh_out);
                  }
									/* close output file */
									fclose(fh_out);

									printf("Output file written.\n");

								}

								/* free memory allocated for input file */
								free(file_data);
							}
						}
						break;
					}
				}
			}
		}
	}

	return nReturnCode;
}

