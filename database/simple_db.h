#ifndef _DB_H
#define _DB_H

#include<fstream>
#include<iostream>
#include<vector>
#include<string>
#include<time.h>
#include<stdio.h>

using namespace std;


#define node_number 256        //һ���ڵ�key�ĸ���
#define File_pointer   int            //�ļ���ָ��

#define search_fault -1




struct Node
{
	int keys[node_number + 1];
	int pointers[node_number + 2];              //pointers leaf��ָ�����value��interiorָ����Ƕ��ӵ�ָ��,Ϊ��ͳһ��Ҷ����key��i����Ӧ����pointer��i��
	int keys_size;
	bool isleaf;

};

struct Data
{
	string name;
	int score;
};

class DB{
public:

	DB(string filename, int flag);
	void DB_close();
	bool DB_fetch(int key);
	bool DB_store(int key, string name, int score, int type);
	void DB_delete(int key);
	void _delete(Node&x,int key);

	void search_in_bpuls(int key);
	
	/*���ѽڵ�*/
	void split_node(Node&node, Node&, int i);

	/*ֱ�Ӳ���*/
	void direct_insert(Node& node, int key, File_pointer pointer);

	/*���뺯�����Ӳ����λ�ò���*/
	void insert_in_bpuls(int key);

	/*����û���Ľڵ�*/
	void insert_in_nonfull(Node& node, int key);

	/*�ڴ����Ϸ���һ��B+���ڵ�ռ���������ݵĿռ�*/
	File_pointer Get_Ptr(string name);

	/*��һ��B+���ڵ�д��address��ַ*/
	void WriteNode(File_pointer address, Node &r);

	/*��ȡaddress��ַ�ϵ�һ��B+���ڵ�*/
	void ReadNode(File_pointer address, Node &r);

	/*��һ������д��address��ַ*/
	void WriteDate(File_pointer address, Data &r);

	/*��ȡaddress��ַ�ϵ�һһ������*/
	void ReadData(File_pointer address, Data &r);

	void case1_4(Node&node,Node&child,int& i,bool is_5);
	void case2_5(Node&node,Node&child,int key,int& i);
	void case3(Node&node);

	
	/*��������*/
	void Visit();

	Data curr_dat;                     //��ȡ����������
	int first_dat;                   //Ҷ�ӽڵ��һ��Ҷ�ӵ�ָ��
private:

	string idx_name;                    //�����ݿ������
	string dat_name;
	FILE* fst_idx;
	FILE* fst_dat;
	File_pointer idxroot_ptr;                //����B+���ĸ���ƫ����     
	File_pointer idx_ptr;                      //������ƫ����
	File_pointer dat_ptr;                      //�ļ���ƫ����	
	Node curr_node;
	Node root_node;                              //���ڵ㣬���ڵ㲻�ö��룬�������ڴ���
	bool is_replace;                 //�����Ƿ��滻



};


/*define the type of store*/
#define insert 1                     //insert new record
#define replace 2                    //replace old record
#define store 3                      //replace or insert

/*define the type of flag*/
#define create 1                       //create a new file with the name of filename
#define open_file  2                        //open the file with the name of filename


#endif