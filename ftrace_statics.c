/*
 * ftrace_statistics.c - Simple tool for statistics ftrace output file
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Author: Mingming, Zhang <mingmingx.zhang@intel.com>
 *                        <fishtown.mm@gmail.com>
 *data struct define
 * ____________
 *|name       |
 *|count      |
 *|do_call_sum|
 *|-----------|    
 *|callee[]   |
 *|-----------|
 *|do_call[]  |
 *|___________|
 *@name : one func called map one node
 *@count: the mapped func called total times
 *@do_call_sum : the mapped func call other funcs total times
 *@callee : mapped func called funcs array, ASC sorted
 *@do_call: mapped func call other funcs array, DESC sorted
 * */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#define MAX_FUNC_NUM 1000
#define FUNC_LEN  80
char buff[1024];
struct callee_array {
		char name[FUNC_LEN];
		int count;
};

struct node {
		char name[FUNC_LEN];
		int  count;
		int  do_call_sum;
		struct callee_array callee[MAX_FUNC_NUM];
		struct callee_array do_call[MAX_FUNC_NUM]; 
		struct node * next;
};



//if success return node else return NULL
struct node* search_list(struct node *head, char *func_called)
{  
	struct node * p;
	
	p = head->next;
	while (p !=NULL && strcmp(p->name, func_called) != 0)
	{
		p = p->next;
	}

	return p;
}

int add_array(struct node* head, struct node* target, char* str, char* called)
{
	int i;
	struct node * p;
	p = head;

	if (target == NULL)
	{
		while (p->next != NULL)
		{
		   p = p->next;
		}
		
		struct node *tail =(struct node *)malloc(sizeof(struct node));
		if (tail == NULL)
		{
			printf("malloc memory failed!\n");
			return -1;
		}
		tail->next =NULL;
		tail->count =1;
        strcpy(tail->name,called); 
		tail->callee[0].count = 1;
        tail->callee[1].count = -1;
        strcpy(tail->callee[0].name,str);
		tail->do_call[0].count = -1;
		tail->do_call_sum =0;

		p->next = tail;
	}
	else
	{
		//search str in target->callee[];
		for(i=0; i< MAX_FUNC_NUM && target->callee[i].count !=-1;i++)
		{
			if(strcmp(str,target->callee[i].name) ==0)
			{
				target->callee[i].count ++;
				//while the last one is bigger ,exchange it with former	
				while (i > 0 && target->callee[i].count > target->callee[i-1].count)
				{
				char tmp[FUNC_LEN];
				strcpy(tmp,target->callee[i].name);
				strcpy(target->callee[i].name,target->callee[i-1].name);
				strcpy(target->callee[i-1].name,tmp);

				target->callee[i].count = target->callee[i].count + target->callee[i-1].count;
				target->callee[i-1].count = target->callee[i].count - target->callee[i-1].count;
				target->callee[i].count = target->callee[i].count - target->callee[i-1].count;
				i--;
				}
				break;
			}
		}

		if(target->callee[i].count == -1)
		{
			target->callee[i+1].count =-1;			
			strcpy(target->callee[i].name,str);
			target->callee[i].count = 1;
		}
		
		target->count ++;

		struct node * swap;
		while (p->next->next != NULL &&p->next->next !=target &&p->next !=target )
		{	
			p = p->next;
		}

		if(p->next->count < target->count)
		{
			swap = p->next;
			p->next = target;
			swap->next = target->next;
			target->next = swap;
		}
	}

			return 0;
}

int main(int argc, char *argv[])
{
	FILE *fp;
	char * str;
	struct node *head;
	struct node *list;
	char func_called[FUNC_LEN];
	char func_callee[FUNC_LEN];
	int ret = 0;
	int i,j;

	if (argc <= 1) {
		printf("Usage: %s file_name \n", argv[0]);
		exit(-1);
	}

	fp = fopen(argv[1], "r");
	if (fp == NULL) {
		printf("open file failed\n");
		exit(-1);
	}
    
	head = (struct node *)malloc(sizeof(struct node));
	head->next  =NULL;
	head->count = -1;

	while (1)
	{
		str = fgets(buff, sizeof(buff), fp);
			if (str ==NULL)
					break;
		while (str[0] =='#')
		{
			str = fgets(buff, sizeof(buff), fp);
		}
		//get func name 
		strcpy(func_called,(strstr(str,": ")+2));
		strcpy(func_callee,(strstr(str,"<-")+2));
		for(i=0;func_callee[i]!='\n';i++);
			func_callee[i]='\0';
		for(i=0;func_called[i]!=' ';i++);
			func_called[i]='\0';

		struct node *pSearch;
		pSearch = search_list(head,func_called);
		if (pSearch != NULL)
		{
			add_array(head,pSearch,func_callee,func_called);
		}
		else
		{
			ret = add_array(head,pSearch,func_callee,func_called);
			if (ret == -1)
			{
				printf("**ERROR**memory is not enough now!\n");
				return 0;
			}
		}
	}
	
//call other
///*
	list = head;
	struct node * curser;
	while (list->next !=NULL)
	{
		list = list->next;
		curser = head->next;

		while (curser != NULL && list !=curser)
		{
			for(i=0;i<MAX_FUNC_NUM &&curser->callee[i].count != -1;i++)
			{
				if(strcmp(list->name,curser->callee[i].name) ==0)
				{
					j=0;
					while (j< MAX_FUNC_NUM &&list->do_call[j].count!=-1)
						j++;
					list->do_call[j].count = curser->callee[i].count;
					strcpy(list->do_call[j].name,curser->name);
					list->do_call[j+1].count = -1;
					list->do_call_sum += curser->callee[i].count;	
				
					while (j>0 && list->do_call[j-1].count < list->do_call[j].count)
					{
						int  tmp_c;
						tmp_c = list->do_call[j].count;
						list->do_call[j].count = list->do_call[j-1].count;
						list->do_call[j-1].count = tmp_c;

						char tmp[FUNC_LEN];
						strcpy(tmp,list->do_call[j-1].name);
						strcpy(list->do_call[j-1].name,list->do_call[j].name);
						strcpy(list->do_call[j].name,tmp);

						j--;
					}
					break;
				}
			}//for
			curser = curser->next;
		}			
	}

//report
	while (head->next !=NULL)
	{
		head=head->next;

		printf("------------------------------------------");
		printf("------------------------------------------\n");

		printf("%50s%9s%20s\n","func","time","percent");
		for(i=0;head->callee[i].count!=-1;i++);
		while (i--)
		{
			printf("%50s%9d%20f%s\n",head->callee[i].name,head->callee[i].count,
			(float)head->callee[i].count*100/head->count,"%");
		}
		printf("%20s  %5s  %5s\n","FUNC","CALLED","CALLEE");
		printf("%20s  %5d  %5d\n",head->name,head->count,head->do_call_sum);

		printf("%50s%9s%20s\n","func","time","percent");
		for(i=0;head->do_call[i].count!=-1;i++)	
		{
			printf("%50s%9d%20f%s\n",head->do_call[i].name,head->do_call[i].count,
							(float)head->do_call[i].count*100/head->do_call_sum,"%");
		}


	}	

	printf("------------------------------------------");
	printf("------------------------------------------\n");
	return 0;
}

