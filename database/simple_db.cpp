
#include"simple_db.h"
#include"test.h"

using namespace std;



DB::DB(string filename, int flag)
{
	idx_name = filename+".idx";
	dat_name = filename + ".dat";

	if (flag == create)
	{
		errno_t err;
		err= fopen_s(&fst_idx, idx_name.c_str(), "wb+");                              //�����ļ�,���ڵĻᱻɾ��
		err = fopen_s(&fst_dat, dat_name.c_str(), "wb+");
		int root = sizeof(int)*2;
		int first = root;
		fwrite((char*)&root,sizeof(root),1,fst_idx);
		fseek(fst_idx,sizeof(int),SEEK_SET );
		fwrite((char*)&first, sizeof(root), 1, fst_idx);
		first_dat = root;
		
		idx_ptr =idxroot_ptr;
		dat_ptr = search_fault;                                         //��ʾû�ж�Ӧ��ƫ����
		curr_node.isleaf = true;                           //�����յĽڵ�
		
		curr_node.keys_size = 0;                           
		WriteNode( 8, curr_node);
		idxroot_ptr = sizeof(int) * 2;

		root_node.isleaf = true;
		root_node.keys_size = 0;

	}
	if (flag == open_file)
	{
		errno_t err;
		err= fopen_s(&fst_idx,idx_name.c_str(), "ab+");
		err= fopen_s(&fst_dat,dat_name.c_str(), "ab+");
		fread((char*)&idxroot_ptr, sizeof(int), 1, fst_idx);      //�������ƫ������
		fseek(fst_idx,sizeof(int),SEEK_SET);
		fread((char*)&first_dat, sizeof(int), 1, fst_idx);
	    idx_ptr = idxroot_ptr;
		dat_ptr = search_fault;

		fseek(fst_idx,idxroot_ptr,SEEK_SET);
		fread((char*)&curr_node, sizeof(Node), 1, fst_idx);

		fseek(fst_idx,idxroot_ptr,SEEK_SET);
		fread((char*)&root_node, sizeof(Node), 1,fst_idx);
		ReadNode( idxroot_ptr, curr_node);        //������Ľڵ㣻

	}
}




/*���Һ��������ҳɹ���db�и���dat_ptr,���ɹ�ʱ��db��idx_ptrҲ���µ���ӦҶ�ӽڵ��ϣ����ڲ���*/

void DB::search_in_bpuls( int key)
{
	int i;
	idx_ptr = idxroot_ptr;
	
	while(true)
	{
		ReadNode(idx_ptr, curr_node);                                              //������Ľڵ�
		for (i = 0; i < curr_node.keys_size&&key > curr_node.keys[i]; ++i){};
		if (i < curr_node.keys_size&&curr_node.isleaf&&key == curr_node.keys[i])
		{
			dat_ptr = curr_node.pointers[i];                                                 
			return;

		}
		if (curr_node.isleaf)
		{
			dat_ptr = search_fault;
			return;
		}
			
		idx_ptr = curr_node.pointers[i];

	} 
	
}


/*���ض�Ӧ��key��value*/
bool DB::DB_fetch( int key)
{
	search_in_bpuls( key);
	if (dat_ptr != search_fault)
	{
		ReadData( dat_ptr, curr_dat);
		return true;
	}
	return false;
}


/*�����ɾ������*/

bool DB::DB_store(int key, string name, int score, int type)
{
	if (type == insert)
	{
		is_replace = false;
		curr_dat.name = name;
		curr_dat.score = score;
		dat_ptr = Get_Ptr(dat_name);                               //dat_ptr�б��������ļ�λ��ָ��
		WriteDate(dat_ptr, curr_dat);

		insert_in_bpuls(key);                                     //�Ӳ���λ�ò��������ļ���ȥ
		return true;
	}

	if (type == replace)
	{
		is_replace = true;

		search_in_bpuls(key);
		if (dat_ptr == search_fault)
			return false;

		//����������
		curr_dat.name = name;
		curr_dat.score = score;
		WriteDate(dat_ptr, curr_dat);

		insert_in_bpuls(key);
		return true;
	}
}




/*���뺯�����Ӳ����λ�ò���*/
void DB::insert_in_bpuls(int key)
{

	idx_ptr = idxroot_ptr;
	ReadNode(idx_ptr, curr_node);

	if (curr_node.keys_size == node_number)
	{

		File_pointer p = Get_Ptr(idx_name);                  //�µĸ�
	
	

		WriteNode(idx_ptr, root_node);                               //ԭ�ȵĸ�д��

		root_node.pointers[0] = idx_ptr;                             //�µĸ�
		root_node.isleaf = false;
		root_node.keys_size = 0;

		WriteNode(p, root_node);                                 //д��

		idxroot_ptr = p;                                       

		idx_ptr = idxroot_ptr;
		split_node(root_node, curr_node, 0);


		insert_in_nonfull(root_node, key);

	}
	else
		insert_in_nonfull(curr_node, key);

}

/*���벻������*/
void DB::insert_in_nonfull(Node& node, int key)
{
	if (node.isleaf)                                                     //ֱ�Ӳ���
	{
		direct_insert(node, key, dat_ptr);
	
	
	    WriteNode(idx_ptr, node);
	}


	else{

		int i;
		for (i = 0; i<node.keys_size&&key > node.keys[i]; ++i){};
		Node child;
		int child_ptr = node.pointers[i];
		ReadNode(child_ptr, child);
		if (child.keys_size == node_number)
		{
			split_node(node, child, i);
			if (key > node.keys[i])
			{
				i = i + 1;
				idx_ptr = node.pointers[i];
				ReadNode(idx_ptr, child);
				insert_in_nonfull(child, key);
				return;
			}
		}
		idx_ptr = child_ptr;
		insert_in_nonfull(child, key);



	}
}


/*��child_node���ѣ�����father_node�ĵ�i��λ��*/
void DB::split_node(Node&father_node, Node&child_node, int i)
{
	int split_key;



	Node new_node;
	File_pointer temp_pointer = Get_Ptr(idx_name);


	if (!child_node.isleaf)
	{
		split_key = child_node.keys[child_node.keys_size / 2];


		new_node.keys_size = (child_node.keys_size - 1) / 2;                                           //�µĽڵ��һЩ����
		for (int j = 0, k = child_node.keys_size / 2 + 1; j < new_node.keys_size; ++j, ++k)
		{
			new_node.keys[j] = child_node.keys[k];
			new_node.pointers[j] = child_node.pointers[k];
		}
		new_node.pointers[new_node.keys_size] = child_node.pointers[child_node.keys_size];
		new_node.isleaf = child_node.isleaf;

		WriteNode(temp_pointer, new_node);


		child_node.keys_size = child_node.keys_size / 2;                                         //֮ǰ�ڵ��һЩ����
		WriteNode(father_node.pointers[i], child_node);
	}
	else{


		split_key = child_node.keys[(child_node.keys_size - 1) / 2];


		new_node.keys_size = child_node.keys_size / 2;                                           //�µĽڵ��һЩ����
		for (int j = 0, k = (child_node.keys_size - 1) / 2 + 1; j < new_node.keys_size; ++j, ++k)
		{
			new_node.keys[j] = child_node.keys[k];
			new_node.pointers[j] = child_node.pointers[k];
		}
		new_node.pointers[new_node.keys_size] = child_node.pointers[child_node.keys_size];
		new_node.isleaf = child_node.isleaf;

		WriteNode(temp_pointer, new_node);


		child_node.keys_size = (child_node.keys_size - 1) / 2 + 1;                                         //֮ǰ�ڵ��һЩ����
		child_node.pointers[node_number + 1] = temp_pointer;                                      //ָ����һλ
		WriteNode(father_node.pointers[i], child_node);
	}

	direct_insert(father_node, split_key, temp_pointer);                                  //���뵽���׽ڵ���
	WriteNode(idx_ptr, father_node);


}

/*ֱ���ڽڵ�node�в��뺯��*/
void DB::direct_insert(Node& node, int key,int pointer)                    
{
	int i;
	for (i = 0; i<node.keys_size&&key > node.keys[i]; ++i){};
	
	if (!node.isleaf)
	{
		
		for (int j = node.keys_size; j > i; --j)
		{
			node.keys[j] = node.keys[j - 1];
			node.pointers[j + 1] = node.pointers[j];
		}
		node.keys[i] = key;
		node.pointers[i + 1] = pointer;
		node.keys_size++;
	}
	else{

		if (i < node.keys_size &&  node.keys[i] == key)  //��B+��Ҷ�ڵ��ҵ�����ͬ�ؼ���  
		{
			//�ؼ��ֲ����ظ�
			if (is_replace)
			{
				node.pointers[i] = pointer;
			}
			return;
		}

		for (int j = node.keys_size; j > i; --j)
		{
			node.keys[j] = node.keys[j - 1];
			node.pointers[j] = node.pointers[j-1];
		}
		node.pointers[node.keys_size + 1] = node.pointers[node.keys_size];
		node.keys[i] = key;
		node.pointers[i] = pointer;
		node.keys_size++;
	}
}



/*�޻���ɾ������֤���ӵ�keys�ǰ�����
  �������
  case 1,�ڵ�ǰ�ڽڵ��ҵ��ؼ��֣��Ҷ�����Ҷ�ڵ�
  case 2,�ڵ�ǰ�ڽڵ��ҵ��ؼ��֣��Ҷ������ڽڵ�
  case 3,�ڵ�ǰҶ�ڵ��ҵ��ؼ���
  case 4,�ڵ�ǰ�ڽڵ�û�ҵ��ؼ��֣��Ҷ�����Ҷ�ڵ�
  case 5,�ڵ�ǰ�ڽڵ�û�ҵ��ؼ��֣��Ҷ������ڽڵ�
  case 6,�ڵ�ǰҶ�ڵ�û�ҵ��ؼ���
*/


/*��1�͵�4�������*/
void DB::case1_4(Node&x, Node& child, int& i, bool is_1)
{
	int j;
	
	if (child.keys_size >node_number / 2)                            //����������
	{
		if (is_1 == true){
			x.keys[i] = child.keys[child.keys_size - 2];
			child.keys_size--;

			WriteNode(idx_ptr, x);                                       //д�����
			WriteNode(x.pointers[i], child);

			return;
		}
	}
	else                                                              //�����ӽڵ�Ĺؼ�������������  
	{
		if (i > 0)                                                     //�����ֵܽڵ�  
		{
			Node lbchild;
			ReadNode(x.pointers[i - 1], lbchild);

			if (lbchild.keys_size > node_number / 2)                    //���ֵ��ǰ����ģ������ֵܽ�һ��  
			{
				for (j = child.keys_size; j > 0; j--)
				{
					child.keys[j] = child.keys[j - 1];
					child.pointers[j] = child.pointers[j - 1];
				}

				child.keys[0] = x.keys[i - 1];
				child.pointers[0] = lbchild.pointers[lbchild.keys_size - 1];

				child.keys_size++;

				lbchild.keys_size--;

				x.keys[i - 1] = lbchild.keys[lbchild.keys_size - 1];

				if (is_1 == true)
				{
					x.keys[i] = child.keys[child.keys_size - 2];                          //�ݹ�ɾ����һ��
				}

				WriteNode(idx_ptr, x);
				WriteNode(x.pointers[i - 1], lbchild);
				WriteNode(x.pointers[i], child);

			}
			else                                                              //���ֵ�û����ʱ�������ֵܺϲ�  
			{
				for (j = 0; j < child.keys_size; j++)
				{
					lbchild.keys[lbchild.keys_size + j] = child.keys[j];
					lbchild.pointers[lbchild.keys_size + j] = child.pointers[j];

				}
				lbchild.keys_size += child.keys_size;

				lbchild.pointers[node_number + 1] = child.pointers[node_number + 1];

				//�ͷ�child�ڵ�ռ�õĿռ�x.pointers[i]  

				for (j = i - 1; j < x.keys_size - 1; j++)
				{
					x.keys[j] = x.keys[j + 1];
					x.pointers[j + 1] = x.pointers[j + 2];
				}
				x.keys_size--;

				if (is_1 == true)
				{
					x.keys[i - 1] = lbchild.keys[lbchild.keys_size - 2];
				}

				WriteNode(idx_ptr, x);
				WriteNode(x.pointers[i - 1], lbchild);

				i--;                                                                         //�ݹ�ɾ��lbchild�еĹؼ���

			}
		}
		else                                                                         //ֻ�����ֵܽڵ�  
		{
			Node rbchild;
			ReadNode(x.pointers[i + 1], rbchild);

			if (rbchild.keys_size > node_number / 2)                                        //�������ֵ�һ�� 
			{
				x.keys[i] = rbchild.keys[0];
				child.keys[child.keys_size] = rbchild.keys[0];

				child.pointers[child.keys_size] = rbchild.pointers[0];

				child.keys_size++;

				for (j = 0; j < rbchild.keys_size - 1; j++)
				{
					rbchild.keys[j] = rbchild.keys[j + 1];
					rbchild.pointers[j] = rbchild.pointers[j + 1];
				}

				rbchild.keys_size--;

				WriteNode(idx_ptr, x);
				WriteNode(x.pointers[i], child);
				WriteNode(x.pointers[i + 1], rbchild);

			}
			else                                                                         //�����ֵܺϲ�
			{
				for (j = 0; j < rbchild.keys_size; j++)
				{
					child.keys[child.keys_size + j] = rbchild.keys[j];
					child.pointers[child.keys_size + j] = rbchild.pointers[j];
				}
				child.keys_size += rbchild.keys_size;

				child.pointers[node_number + 1] = rbchild.pointers[node_number + 1];

				//�ͷ�rbchildռ�õĿռ�x.pointers[i+1]  

				for (j = i; j < x.keys_size - 1; j++)
				{
					x.keys[j] = x.keys[j + 1];
					x.pointers[j + 1] = x.pointers[j + 2];
				}
				x.keys_size--;

				WriteNode(idx_ptr, x);
				WriteNode(x.pointers[i], child);

			}

		}

	}
}


/*��2�͵�5�������*/
void DB::case2_5(Node&x,Node&child,int key, int& i){
	//�ҵ�key��B+��Ҷ�ڵ�����ֵܹؼ���,������ؼ���ȡ��key��λ��  
	int j;


	if (child.keys_size > node_number / 2)                                                 //���ӵĹؼ��ִ��ڹ��룬ֱ������
	{
	}
	else                                                          //�����ӽڵ�Ĺؼ�������������,���ֵܽڵ��ĳһ���ؼ�����������  
	{
		if (i > 0)                                                  //x.key[i]�����ֵ�  
		{
			Node lbchild;
			ReadNode(x.pointers[i - 1], lbchild);

			if (lbchild.keys_size > node_number / 2)                //�����ֵ��ӽ裬���ֵ�Ϊ�ڽڵ�
			{
				for (j = child.keys_size; j > 0; j--)
				{
					child.keys[j] = child.keys[j - 1];
					child.pointers[j + 1] = child.pointers[j];
				}
				child.pointers[1] = child.pointers[0];
				child.keys[0] = x.keys[i - 1];
				child.pointers[0] = lbchild.pointers[lbchild.keys_size];

				child.keys_size++;

				x.keys[i - 1] = lbchild.keys[lbchild.keys_size - 1];
				lbchild.keys_size--;

				WriteNode(idx_ptr, x);
				WriteNode(x.pointers[i - 1], lbchild);
				WriteNode(x.pointers[i], child);



			}
			else                                                  //  �����ֵܺϲ�
			{
				lbchild.keys[lbchild.keys_size] = x.keys[i - 1];   //�����ӽڵ㸴�Ƶ������ֵܵ�ĩβ  
				lbchild.keys_size++;

				for (j = 0; j < child.keys_size; j++)      //��child�ڵ㿽����lbchild�ڵ��ĩβ,  
				{
					lbchild.keys[lbchild.keys_size + j] = child.keys[j];
					lbchild.pointers[lbchild.keys_size + j] = child.pointers[j];
				}
				lbchild.pointers[lbchild.keys_size + j] = child.pointers[j];
				lbchild.keys_size += child.keys_size;        //�Ѿ���child������lbchild�ڵ�  


				//�ͷ�child�ڵ�Ĵ洢�ռ�,x.pointers[i]  


				//���ҵ��ؼ��ֵĺ���child��ؼ������ֵܵĺ���lbchild�ϲ���,���ùؼ���ǰ��,ʹ��ǰ�ڵ�Ĺؼ��ּ���һ��  
				for (j = i - 1; j < x.keys_size - 1; j++)
				{
					x.keys[j] = x.keys[j + 1];
					x.pointers[j + 1] = x.pointers[j + 2];
				}
				x.keys_size--;

				WriteNode(idx_ptr, x);
				WriteNode(x.pointers[i - 1], lbchild);

				i--;

			}

		}
		else                                                                  //����x.key[i]ֻ�����ֵ�  
		{
			Node rbchild;
			ReadNode(x.pointers[i + 1], rbchild);

			if (rbchild.keys_size > node_number / 2)                                            //���ڽڵ�����ֵܽ�
			{

				child.keys[child.keys_size] = x.keys[i];
				child.keys_size++;

				child.pointers[child.keys_size] = rbchild.pointers[0];
				x.keys[i] = rbchild.keys[0];

				for (j = 0; j < rbchild.keys_size - 1; j++)
				{
					rbchild.keys[j] = rbchild.keys[j + 1];
					rbchild.pointers[j] = rbchild.pointers[j + 1];
				}
				rbchild.pointers[j] = rbchild.pointers[j + 1];
				rbchild.keys_size--;

				WriteNode(idx_ptr, x);
				WriteNode(x.pointers[i], child);
				WriteNode(x.pointers[i + 1], rbchild);

			}
			else                                                             //  �����ֵܺϲ�
			{
				child.keys[child.keys_size] = x.keys[i];
				child.keys_size++;

				for (j = 0; j < rbchild.keys_size; j++)                 //��rbchild�ڵ�ϲ���child�ڵ��  
				{
					child.keys[child.keys_size + j] = rbchild.keys[j];
					child.pointers[child.keys_size + j] = rbchild.pointers[j];
				}
				child.pointers[child.keys_size + j] = rbchild.pointers[j];

				child.keys_size += rbchild.keys_size;

				//�ͷ�rbchild�ڵ���ռ�õĿռ�,x,pointers[i+1]  

				for (j = i; j < x.keys_size - 1; j++)    //��ǰ���ؼ���֮��Ĺؼ�������һλ,ʹ�ýڵ�Ĺؼ���������һ  
				{
					x.keys[j] = x.keys[j + 1];
					x.pointers[j + 1] = x.pointers[j + 2];
				}
				x.keys_size--;

				WriteNode(idx_ptr, x);
				WriteNode(x.pointers[i], child);

			}

		}
	}

}

void DB::_delete(Node&x,int key)
{
	int i, j;


	for (i = 0; i < x.keys_size && key > x.keys[i]; i++);

	/*case 1,�ڵ�ǰ�ڽڵ��ҵ��ؼ��֣��Ҷ�����Ҷ�ڵ�*/
	if (i < x.keys_size && x.keys[i] == key)                                       //�ڵ�ǰ�ڵ��ҵ��ؼ���  
	{
		if (!x.isleaf)                                                            //���ڽڵ��ҵ��ؼ���  
		{
			Node child;
			ReadNode(x.pointers[i], child);

			if (child.isleaf)                                                     //���������Ҷ�ڵ�  
			{
				case1_4(x, child, i, true);
			}

			/*case 2,�ڵ�ǰ�ڽڵ��ҵ��ؼ��֣��Ҷ������ڽڵ�*/
			else                                                                             //��ǰ�ؼ��ֵĺ������ں���
			{
				//�ҵ�key��B+��Ҷ�ڵ�����ֵܹؼ���,������ؼ���ȡ��key��λ��  
				int temp = idx_ptr;
				int last_ptr;
				search_in_bpuls(key);
				last_ptr = idx_ptr;
				idx_ptr = temp;
				Node last;

				ReadNode(last_ptr, last);
				x.keys[i] = last.keys[last.keys_size - 2];
				WriteNode(idx_ptr, x);

				case2_5(x,child, key, i);
			}

			idx_ptr = x.pointers[i];                                                //���µݹ�
			_delete(child,key);
		}

		/*case 3,�ڵ�ǰҶ�ӽڵ��ҵ��ؼ���*/
		else                                                                   //��ǰ��Ҷ�ӽڵ㣬ֱ��ɾ�� 
		{
			for (j = i; j < x.keys_size - 1; j++)
			{
				x.keys[j] = x.keys[j + 1];
				x.pointers[j] = x.pointers[j + 1];                            
			}
			x.keys_size--;

			WriteNode(idx_ptr, x);

			return;
		}
	}

	  /*case 4,�ڵ�ǰ�ڽڵ�û�ҵ��ؼ��֣����ӽڵ�����ڵ�*/
	else                                                                         //�ڵ�ǰ�ڵ�û�ҵ��ؼ��֣��������ϣ����µݹ�     
	{
		if (!x.isleaf)        //û�ҵ��ؼ���,��ؼ��ֱ�Ȼ��������pointers[i]Ϊ����������  
		{
			Node child;
			ReadNode(x.pointers[i], child);
			if (child.isleaf)                                                      //����亢�ӽڵ�����ڵ�  
			{
				case1_4(x, child, i, false);
			}

			/*case 5,�ڵ�ǰ�ڽڵ�û�ҵ��ؼ��֣����ӽڵ�����ڵ�*/
			else                                                        //�����亢�ӽڵ����ڽڵ�  
			{
				case2_5(x, child, key, i);
			}
			/*case 6 ��ǰҶ�ӽڵ�û�ҵ��ؼ��֣����ò�����û���������*/
			idx_ptr = x.pointers[i];
			_delete(child,key);
		}
	}
}

void    DB::DB_delete(int key)                                             //��B+��ɾ��һ���ؼ���  
{
	idx_ptr = idxroot_ptr;
	Node x;
	ReadNode(idx_ptr, x);
	_delete(x,key);

	

	if (!root_node.isleaf && root_node.keys_size == 0)    //���ɾ���ؼ��ֺ���ڵ㲻��Ҷ�ڵ㣬���ҹؼ�������Ϊ0ʱ���ڵ�ҲӦ�ñ�ɾ��  
	{
		//�ͷ�ROOT�ڵ�ռ�õĿռ�                                                
		File_pointer temp = root_node.pointers[0];         //���ڵ㷢���ı�
		ReadNode(root_node.pointers[0], root_node);
		idxroot_ptr =temp;         //���ڵ�����,B+���߶ȼ�1  

	}

}


void DB::DB_close()
{
	fseek(fst_idx,0,SEEK_SET);
	fwrite((char*)&idxroot_ptr, sizeof(int), 1, fst_idx);
	fseek(fst_idx,sizeof(int),SEEK_SET);
	fwrite((char*)&first_dat, sizeof(int), 1, fst_idx);
	fseek(fst_idx,idxroot_ptr,SEEK_SET );
	fwrite((char*)&root_node, sizeof(Node), 1, fst_idx);
	fclose(fst_idx);
	fclose(fst_dat);

}




File_pointer DB::Get_Ptr(string filename)
{
	
	if (filename == idx_name)
	{
		fseek(fst_idx,0,SEEK_END );
		return (ftell(fst_idx));
		
	}
	else{
		fseek(fst_dat, 0, SEEK_END);
		return (ftell(fst_dat));
	}
	
}


void  DB::WriteNode(File_pointer address, Node &r)
{
	if (address == idxroot_ptr)
	{
		root_node = r;
		return;
	}
	fseek(fst_idx,address,SEEK_SET);
	fwrite((char*)(&r), sizeof(Node), 1, fst_idx);


}


void  DB::ReadNode(File_pointer address, Node& r)
{
	if (address == idxroot_ptr)
	{
		r = root_node;
		return;
	}
	fseek(fst_idx, address, SEEK_SET);
	fread((char*)(&r), sizeof(Node), 1, fst_idx);
	
	
}


void  DB::WriteDate(File_pointer address, Data &r)
{
	
	fseek(fst_dat, address, SEEK_SET);
	fwrite((char*)(&r), sizeof(Data), 1, fst_dat);
	
}


void  DB::ReadData(File_pointer address, Data &r)
{
	
	fseek(fst_dat, address, SEEK_SET);
	fread((char*)(&r), sizeof(Data), 1, fst_dat);

	

}

void DB::Visit()
{
	int adress = first_dat;
	while (true)
	{
		Node node;

		ReadNode(adress, node);
		for (int i = 0; i < node.keys_size; ++i)
		{
			
		}
		if (node.pointers[node_number + 1] <= 0)
			break;
		adress = node.pointers[node_number + 1];
	}
}
void main()
{

	use();
	

	//��ȡ���Ե�����
	/*DB db("8", create);


	int number = 2000000;
	clock_t time = clock();
	for (int i = 0; i <number; ++i)
	{
		db.DB_store(i, "a", i, insert);
	}
	cout << clock() - time<<endl;
	time = clock();


	int n = 10000;
	for (int i = 0; i < n; ++i)
	{
		db.DB_store(i + number, "keke", 100, insert);
	}
	cout << clock() - time << endl;
	time = clock();


	for (int i = 0; i < n; ++i)
	{
		db.DB_store(i + number, "kekeke", 10, replace);
	}
	cout << clock() - time << endl;
	time = clock();


	for (int i = 0; i < n; ++i)
	{
		db.DB_fetch(i + number);
	}

	cout << clock() - time << endl;
	time = clock();

	
	for (int j = 0; j < n; ++j)
	{
		db.DB_delete(j + number);
	}

	cout << clock() - time << endl;
	time = clock();*/
	/*for (int i = 1; i <= 100000; ++i)
	{
		db.DB_store(i, "keke", 100, insert);

	}
	

	for (int i =10; i <= 99990; ++i){
		db.DB_delete(i);
		
	}
	for (int i = 1; i <=20; ++i)
	{
	if (db.DB_fetch(i) == true)
	{
	cout <<i<<"   "<< db.curr_dat.name << "  " << db.curr_dat.score << endl;
	}
	else
		cout << i << "   " << "search false" << endl;

	}
	for (int i = 99980; i <=100000; ++i)
	{
		if (db.DB_fetch(i) == true)
		{
			cout <<i<<"   "<< db.curr_dat.name << "  " << db.curr_dat.score << endl;
		}
		else
			cout <<i<<"   "<< "search false" << endl;

	}

	cout << endl;
	cout << "�������е�����:" << endl;
	int adress =db.first_dat;
	while (true)
	{
		Node node;

		db.ReadNode(adress, node);
		for (int i = 0; i < node.keys_size; ++i)
		{
			cout << node.keys[i]<<endl;
		}
		if (node.pointers[node_number + 1] <= 0)
			break;
		adress = node.pointers[node_number + 1];
	}*/
	

	

	
	//db.DB_close();

}
