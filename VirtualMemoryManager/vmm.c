#include<stdio.h>
#include<stdlib.h>

#define PAGE_NUMBER_MASK 0xFF00		// bits 8-15 give the page number
#define PAGE_OFFSET_MASK 0xFF 		// bits 0-7 give the page offset
#define PAGE_SIZE 256			// page size = frame size = 256 bytes
#define NUM_OF_ENTRIES_IN_PAGE_TABLE 256 	// 256 entries in page table
#define TOTAL_NUM_OF_FRAMES 256		// 256 frames in main memory
#define NUM_OF_ENTRIES_IN_TLB 16	// 16 entries in TLB

int TLB[NUM_OF_ENTRIES_IN_TLB][2];
int PAGE_TABLE[NUM_OF_ENTRIES_IN_PAGE_TABLE];
int PHYSICAL_MEMORY[TOTAL_NUM_OF_FRAMES][PAGE_SIZE];
int TIME[NUM_OF_ENTRIES_IN_PAGE_TABLE];
int num_of_address_references=0;
int num_of_page_faults=0;
int num_of_TLB_hits=0;
int first_free_frame=0;

/* This function initialises the page table as initially no page is in memory */
void init_page_table()
{	for(int i=0;i<NUM_OF_ENTRIES_IN_PAGE_TABLE;i++)
		PAGE_TABLE[i]=-1;
}

/* This function initialises the TLB as initially no entry in TLB */
void init_TLB()
{	for(int i=0;i<NUM_OF_ENTRIES_IN_TLB;i++)	
	{	TLB[i][0]=-1;
		TLB[i][1]=-1;
	}
}

/* This function returns frame number associated with page number if found in TLB */
int search_TLB(int page_number)
{	for(int i=0;i<NUM_OF_ENTRIES_IN_TLB;i++)
	{	if(page_number==TLB[i][0])
		{	num_of_TLB_hits++;
			TIME[page_number]=num_of_address_references;
			return TLB[i][1];
		}
	}
	return -1;
}

/* This function returns frame number associated with page number if found in page table */
int search_page_table(int page_number)
{	if(PAGE_TABLE[page_number]==-1)
	{	num_of_page_faults++;
		return -1;
	}
	else
		return PAGE_TABLE[page_number];
}

/* This function updates the TLB using LRU policy after after page fault is serviced */
void update_TLB(int page_number,int frame_number)
{	for(int i=0;i<NUM_OF_ENTRIES_IN_TLB;i++)	//if free slot is available in TLB then simply insert  
	{	if(TLB[i][0]==-1)						//page number and frame number in TLB
		{	TLB[i][0]=page_number;
			TLB[i][1]=frame_number;
			TIME[page_number]=num_of_address_references;
			return;
		}
	}
	int min=0;
	for(int i=1;i<NUM_OF_ENTRIES_IN_TLB;i++)	//search for least recently used page in TLB
	{	if(TIME[TLB[i][0]]<TIME[TLB[min][0]])
			min=i;
	}
	TLB[min][0]=page_number;					//evict the LRU page and insert new page number and frame number in TLB
	TLB[min][1]=frame_number;
	TIME[page_number]=num_of_address_references;
	return;
}

/* Main function starts here */
int main(int argc,char *argv[])
{	if(argc!=2)
	{	fprintf(stderr,"usage: ./a.out addresses.txt \n");
		return -1;
	}

	init_page_table();
	init_TLB();
	FILE *fp1;
	fp1=fopen("addresses.txt","r");
	if(fp1==NULL)
	{	fprintf(stderr,"%s failed to open\n",argv[1]);
		return -1;
	}
	FILE *fp2;
	fp2=fopen("BACKING_STORE.bin","rb");
	if(fp2==NULL)
	{	fprintf(stderr,"BACKING_STORE.bin failed to open\n");
		return -1;
	}
	char address[20];
	signed char buffer[PAGE_SIZE];
	int virtual_address,page_number,page_offset,frame_number;

	while(fgets(address,20,fp1)!=NULL)
	{	virtual_address=atoi(address);
		page_number=(virtual_address & PAGE_NUMBER_MASK)>>8;		// getting the page number
		page_offset=virtual_address & PAGE_OFFSET_MASK;		// getting the page offset
		num_of_address_references++;
		frame_number=search_TLB(page_number);

		if(frame_number==-1)	// TLB miss occurs
		{	frame_number=search_page_table(page_number);
			if(frame_number!=-1)							// if page is found in page table update TLB
				update_TLB(page_number,frame_number);
		}

		if(frame_number==-1)	// page fault occurs
		{	if(fseek(fp2,page_number*PAGE_SIZE,SEEK_SET)!=0)	// go to page location in BACKING STORE
			{	fprintf(stderr,"error executing fseek\n");
				return -1;
			}
			fread(buffer,sizeof(signed char),PAGE_SIZE,fp2);	// read from BACKING STORE 
			for(int i=0;i<PAGE_SIZE;i++)
				PHYSICAL_MEMORY[first_free_frame][i]=buffer[i];	// store in physical memory
			frame_number=first_free_frame;
			first_free_frame++;
			PAGE_TABLE[page_number]=frame_number;		// updating page table
			update_TLB(page_number,frame_number);		// updating TLB
		}

		printf("logical address : %d ",virtual_address);
		printf("physical address : %d ",(frame_number*PAGE_SIZE)+page_offset);
		printf("value : %d \n",PHYSICAL_MEMORY[frame_number][page_offset]);
	}

	double page_fault_rate=(double)num_of_page_faults/(double)num_of_address_references;	//Page­fault rate
	double TLB_hit_rate=(double)num_of_TLB_hits/(double)num_of_address_references;			//TLB hit rate
	printf("Page­fault rate : %lf (%.2lf%%)\n",page_fault_rate,100*page_fault_rate);
	printf("TLB hit rate : %lf (%.2lf%%)\n",TLB_hit_rate,100*TLB_hit_rate);
	fclose(fp1);
	fclose(fp2);

	return 0;
}