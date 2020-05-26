#include <stdio.h>
#include<iostream>
#include <string.h>  // For memcmp()
#include <stdlib.h>  // For exit()
#include <cstdlib>
#include <vector>
#include <chrono> 
#include "lz4.h" 
#include "/home/louay/Downloads/File-Vector-master/file_vector.hpp"
using namespace std;

template<typename T,int C>
class CompDec{
    
    int 			 	 Num_chunks;//Number of chunks from the big vector
    int 				 total_pages;//number of pages
    size_t 			 	 size;//Size of input array
    int		 		 	 _chunksize=C;//Number of elements in each chunk
    vector<T*>			 chunks;//Splitted Arrays
    vector<int>			 chunks_sizes;//size of each chunk
    vector<char*>		 Compressed_chunks;//Vector of compressed chunks
    vector<T*>			 DeCompressed_chunks;//Vector of Decompressed chunks
    vector<int>			 compressed_chunks_sizes;//Vector of compressed chunks' sizes
    vector<int>			 Decompressed_chunks_sizes;//Vector of Decompressed chunks' sizes
    vector<const char*>	 Splitted_Compressed_chunks;//Vector of Splitted compressed chunks
    vector<size_t>	 	 Splitted_Compressed_chunks_sizes;
    int					 page_size;
    ZoneMapSet<T>		 zonemaps;
    
    public:
    
    //Constructor
    CompDec(T* ch, size_t ssize,int pg_sz)
    {  
    	page_size=pg_sz;
        size=ssize;
        if(ssize%_chunksize==0)
            	{Num_chunks=ssize/_chunksize;}
        else if (ssize%_chunksize!=0)
            	{Num_chunks=(ssize/_chunksize)+1;}
        chunks=Split_Array(ch,ssize,_chunksize);
        //Create zone_maps
        ZoneMapSet<T>zonemapss(C*sizeof(T),ssize);
        this->zonemaps=zonemapss;
        zonemaps.InitFromData(ch, ssize);
        Compress_chunks();
        Decompress_chunks();
          

    }
    
    int Get_num_chunks()
    {return Num_chunks;}
    
    
    //Compress chunks Method
    void Compress_chunks()
    { for(int i=0;i<Num_chunks-1;i++)
   		    {//Allocate memory
    		char src[C*sizeof(T)];
    		//Copy given string into allocated memory
			memcpy(src,chunks[i],_chunksize*sizeof(T));
			//Max size for compression
			
			const int max_dst_size = LZ4_compressBound(_chunksize*sizeof(T));
		
			//Allocate memory for compressed chunk
    		char *dest = new char[(size_t)max_dst_size];
			if(dest==NULL)
				run_screaming("Failed to allocate memory for *compressed_data.", 1);
			const int compressed_data_size = LZ4_compress_default(src, dest, _chunksize*sizeof(T), max_dst_size);
			compressed_chunks_sizes.emplace_back(compressed_data_size);
			Compressed_chunks.emplace_back(dest);	
	     	}
	     	
	     	/*******************************************************/
			//Last Chunk compression
			char last_src[(size-(_chunksize*(Num_chunks-1)))*sizeof(T)];
			
    		//Copy given string into allocated memory
			memcpy(last_src,chunks[Num_chunks-1],(size-(_chunksize*(Num_chunks-1)))*sizeof(T));
			//Max size for compression
			
			const int max_dst_size = LZ4_compressBound((size-(_chunksize*(Num_chunks-1)))*sizeof(T));
			//Allocate memory for compressed chunk
    		char *dest = new char[(size_t)max_dst_size];
			if(dest==NULL)
				run_screaming("Failed to allocate memory for *compressed_data.", 1);
			const int compressed_data_size = LZ4_compress_default(last_src, dest, (size-(_chunksize*(Num_chunks-1)))*sizeof(T), max_dst_size);
			compressed_chunks_sizes.emplace_back(compressed_data_size);
			Compressed_chunks.emplace_back(dest);	
		
		
    }
    void gt_dec(){for (int i=0;i<Num_chunks;i++)cout<<Decompressed_chunks_sizes[i]<<endl;}
    void Decompress_chunks()
    {
		for(int i=0;i<Num_chunks;i++)
		{
		 //Allocate memory for regen_buffer
    	 char* regen_buffer = new char [_chunksize*sizeof(T)];
         if(regen_buffer==NULL)
		 {run_screaming("Failed to allocate memory for *Decompressed_data.", 1);}
		 const int decompressed_size = LZ4_decompress_safe(Compressed_chunks[i], regen_buffer, compressed_chunks_sizes[i], _chunksize*sizeof(T));
		 Decompressed_chunks_sizes.emplace_back(decompressed_size);//Save decompressed chunks' sizes into a vector
		 DeCompressed_chunks.emplace_back((T*)regen_buffer);//Save decompressed chunks into a vector
		}
    }
    
	//Find data using full scan and return first match
	size_t Find(T Key)
	{int offset;
	bool found_it=false;
	//Decompress_chunks();
	for(int i=0;i<Num_chunks;i++)
		{
		for (unsigned int j = 0; j < Decompressed_chunks_sizes[i]/sizeof(T); j++)
			{
			if(Key==DeCompressed_chunks[i][j])
				{
				offset= i*(Decompressed_chunks_sizes[i]/sizeof(T))+j;
				found_it=true;
				return offset;
				break;
				}
			}
		if(found_it)
                break;
		}
	}
	
	//Find data using zone_maps and return first match
    size_t Find(T key, bool use_zone_maps)
    {
    bool found_it = false;
    for(size_t i=0; i<zonemaps.Size(); i++) {
    		auto zm = zonemaps.Get(i);
            if(zm->Intersects(key, key)) {
                for(size_t j=zm->start_loc; j<zm->end_loc; j++) {
                    if(DeCompressed_chunks[i][j-(i*_chunksize)] == key) {
                        
                        found_it = true;
                        return j; 
                        break;
                    }
                }
            }
            if (found_it)
            break;
           
        }
    }
    
    void Finds(T key, bool use_zone_maps,int&loc)
    {
    bool found_it = false;
    for(size_t i=0; i<zonemaps.Size(); i++) {
    		auto zm = zonemaps.Get(i);
            if(zm->Intersects(key, key)) {
            	T* decompressed_chunk=Decompress_chunk(i);
                for(size_t j=zm->start_loc; j<zm->end_loc; j++) {
                    if(decompressed_chunk[j-(i*_chunksize)] == key) {
                        
                        found_it = true;
                        loc=j;
                        //return j; 
                        break;
                    }
                }
            }
            if (found_it)
            break;
         }
    if(!found_it){
    	cout<<"not found"<<endl;
    //loc=-1;
   		}
    }
    size_t Find_after(T key, bool use_zone_maps,size_t &sz,int total_pages,int&begin,int&end)
    {
    bool found_it = false;
    for(size_t i=0; i<zonemaps.Size(); i++) {
    		auto zm = zonemaps.Get(i);
            if(zm->Intersects(key, key)) {
            int num_pages;
            if(compressed_chunks_sizes[0]%page_size==0)
            num_pages=compressed_chunks_sizes[0]/page_size;
            else
            num_pages=(compressed_chunks_sizes[0]/page_size)+1;
            //added here
            //int begin,end;
            if(i!=Num_chunks-1){
                begin=i*num_pages;
                end=(i+1)*num_pages;
            }
            else{
            	begin=(Num_chunks-1)*num_pages;
                end=total_pages-begin;
            }
            //to here
            char *regen_buffer1=new  char[chunks_sizes[i]*sizeof(int)];
		 	char* new_char1=new char[compressed_chunks_sizes[i]];
		 	new_char1=join(begin,end-1,compressed_chunks_sizes[i],Splitted_Compressed_chunks,Splitted_Compressed_chunks_sizes);
		 	sz=LZ4_decompress_safe(new_char1,regen_buffer1, compressed_chunks_sizes[i], chunks_sizes[i]*sizeof(int));
            
            
            
            	T* decompressed_chunk=(T*)regen_buffer1;
                for(size_t j=zm->start_loc; j<zm->end_loc; j++) {
                    if(decompressed_chunk[j-(i*_chunksize)] == key) {
                        
                        found_it = true;
                        return j; 
                        break;
                    }
                }
            }
            if (found_it)
            break;
         }
    }
    
    //Destructor
    ~CompDec()
    {           
        chunks.clear();
        Compressed_chunks.clear();
        DeCompressed_chunks.clear();
        compressed_chunks_sizes.clear();
        Decompressed_chunks_sizes.clear();
        Splitted_Compressed_chunks_sizes.clear();
        
    chunks_sizes.clear();//size of each chunk
  
    
   
    
   	 Splitted_Compressed_chunks.clear();//Vector of Splitted compressed chunks
    
    }
    
    //Function to Split the Array to little chunks depending on given _chunksize
    vector<T*>Split_Array(const T* Tab,size_t n, int _chunksize)
    {
     vector<int*>chunks;
     
     int Num_chunks = (n - 1) / _chunksize + 1;
     //Num_chunks=n/_chunksize;
     
     for (int i=0; i <Num_chunks-1; i++) //chunks=n/_chunksize
         {  
          int *new_chunk=new int[_chunksize];
          for (int j=0; j<_chunksize; j++) 
              { 
               new_chunk[j]=Tab[j+i*_chunksize];
              }
               chunks.emplace_back(new_chunk);
          	   chunks_sizes.emplace_back(_chunksize);
         }
      /******************Fill Last chunk **************/
      int *last_chunk=new int[n-(_chunksize*(Num_chunks-1))];
      for (int j=0; j<n-(_chunksize*(Num_chunks-1)); j++) 
          {
          last_chunk[j]=Tab[j+(Num_chunks-1)*_chunksize];
          }
          chunks.emplace_back(last_chunk);
          chunks_sizes.emplace_back(n-(_chunksize*(Num_chunks-1)));
      return chunks;
    }
        
    //Return the chunks after splitting the array
    vector<T*> Get_Chunks()
    {
        return (chunks);
    }
    
    //Return the chunks' sizes
    vector<int>Get_chunks_sizes()
    {
    	return chunks_sizes;
    }
	//Return the compressed chunks 
    vector<char*> Get_Compressed_Chunks()
    {
        return (Compressed_chunks);
    }
    
    //Return the Decompressed chunks 
	vector<T*> Get_DeCompressed_Chunks()
    {
        return (DeCompressed_chunks);
    }    
	//Data ended with , Print the first j data from each chunk
	void Get_data(int j)
	{
		for(int i=0;i<Num_chunks;i++)
		{
			cout<<"\nthe first "<<j<<" data ended with from chunk "<<i<<" is"<<endl;
			for(int k=0;k<j;k++)
			cout<<DeCompressed_chunks[i][k]<<" ";
			
		}
		cout<<endl;
	}
	
	//Function to split the compressed chunks into little chunks
	vector<size_t>Split_compressed(int &num)
	{
		num=0;
		for(int i=0;i<Num_chunks;i++)
		{
		 if(page_size<=compressed_chunks_sizes[i])
		 {
		 vector<const char*>compressed;
		 vector<size_t>sizes;
		 compressed=Split_Array(Compressed_chunks[i],(compressed_chunks_sizes[i]),page_size,sizes);
		 for (auto j = compressed.begin(); j != compressed.end(); ++j) 
         	{Splitted_Compressed_chunks.emplace_back(*j);
		  	 num+=1;
			}
			for (auto j = sizes.begin(); j != sizes.end(); ++j) 
         	{Splitted_Compressed_chunks_sizes.emplace_back(*j);
		  	 
			}
			
         }
         else 
         run_screaming("Failed to split compressed chunks, size of page is bigger thank chunk's size",1);
	    }
		return Splitted_Compressed_chunks_sizes ;
	}
	
	/*********************************************************/
	//Redefinition of Split_array for char* type
	vector<char*>Split_Compressed_Arrays(const char* Tab,size_t n, int _chunksize)
    {
     vector<char*>chunks;
     int Num_chunks = (n - 1) / _chunksize + 1;
          
     for (int i=0; i <Num_chunks-1; i++) //chunks=n/_chunksize
         {  
          char new_chunk[_chunksize];
		  memcpy(new_chunk,Tab+i*_chunksize,_chunksize);
          chunks.push_back(new_chunk);
         }
      /******************Fill Last chunk **************/
      char last_chunk[n-(_chunksize*(Num_chunks-1))];
      memcpy(last_chunk,Tab+((Num_chunks-1)*_chunksize),n-(_chunksize*(Num_chunks-1)));
      chunks.push_back(last_chunk);
        
      return chunks;
    }
    //another def
    vector<const char*>Split_Array(const char* Tab,size_t n, int _chunksize,vector<size_t>&v)
    {
     vector<const char*>chunks;
     int Num_chunks = (n - 1) / _chunksize + 1;
     for (int i=0; i <Num_chunks-1;++i) 
         {  
          char *new_chunk=new char[_chunksize];
          for (int j=0; j<_chunksize; ++j) 
              { 
               new_chunk[j]=Tab[j+i*_chunksize];
              }
               chunks.emplace_back(new_chunk);
          	   v.push_back(_chunksize);
         }
      /******************Fill Last chunk **************/
      char *last_chunk=new char[n-(_chunksize*(Num_chunks-1))];
      for (int j=0; j<n-(_chunksize*(Num_chunks-1)); j++) 
          {
          last_chunk[j]=Tab[j+(Num_chunks-1)*_chunksize];
          }
          
          chunks.emplace_back(last_chunk);
          v.push_back(n-(_chunksize*(Num_chunks-1)));
         
      return chunks;
    }//end of def
   
    
    
    /***Join splitted chunk*/
	
	char* join()
	{
	char *joined=new char[compressed_chunks_sizes[0]];
	
    
    
    for(int i=0;i<19;i++)
        {
        strcat(joined,Splitted_Compressed_chunks[i]);
    	}
    
    return joined;
    
	}
	vector<const char*>get_split()
	{
	return(Splitted_Compressed_chunks);
	}
	const char*get_split(int i)
	{
	return(Splitted_Compressed_chunks[i]);
	}
	size_t get_size()
	{return(Splitted_Compressed_chunks.size() );}
	
    //Error Function
 	void run_screaming(const char* message, const int code) 
	{
  		cout<<message<<endl;
  		exit(code);
	}
	
	void print_ratio()
	{
		for (int i=0; i <Num_chunks; i++)
		{
			cout<<"ratio of chunk Num "<<i+1<<"= "<<(float) compressed_chunks_sizes[i]/(_chunksize*sizeof(T))<<endl;
			
		}
	}
	
	//Verification Function
	void Verify_operation()
	{
		for(int i=0;i<Num_chunks-1;i++)
		{
		if(strcmp((char*)DeCompressed_chunks[i],(char*)chunks[i])==0 && Decompressed_chunks_sizes[i]==_chunksize*sizeof(T))
		{
		cout<<"Validation done for chunk number "<<i+1<<endl;
		cout<<"ratio of chunk Num "<<i+1<<"= "<<(float) compressed_chunks_sizes[i]/(_chunksize*sizeof(T))<<endl;
		}
		else	
		run_screaming("Failed to regenerate original data",1);
		}	
		if(strcmp((char*)DeCompressed_chunks[Num_chunks-1],(char*)chunks[Num_chunks-1])==0 && Decompressed_chunks_sizes[Num_chunks-1]==(size-(_chunksize*(Num_chunks-1)))*sizeof(T))
		{
		cout<<"Validation done for chunk number "<<Num_chunks<<endl;
		cout<<"ratio of chunk Num "<<Num_chunks<<"= "<<(float) compressed_chunks_sizes[Num_chunks-1]/((size-(_chunksize*(Num_chunks-1)))*sizeof(T))<<endl;
		}
		else	
		run_screaming("Failed to regenerate original data",1);
	}
	
	//Function to compress each chunk aside
	char* Compress_chunk(int i)
	{
			char *src = new char[_chunksize*sizeof(T)];
    		//Copy given string into allocated memory
			memcpy(src,chunks[i],_chunksize*sizeof(T));
			//Max size for compression
			size_t max_dst_size;
			max_dst_size = LZ4_compressBound(_chunksize*sizeof(T));
			//Allocate memory for compressed chunk
    		char *dest = new char[(size_t)max_dst_size];
			if(dest==NULL)
				run_screaming("Failed to allocate memory for *compressed_data.", 1);
			LZ4_compress_default(src, dest, _chunksize*sizeof(T), max_dst_size);
			return (dest);
			
	}
	
	//Function To decompress each chunk aside
	T* Decompress_chunk(int i)
	{
		 //Allocate memory for regen_buffer
    	 char *regen_buffer=new  char[_chunksize*sizeof(T)];
         if(regen_buffer==NULL)
		 {run_screaming("Failed to allocate memory for *Decompressed_data.", 1);}
		 LZ4_decompress_safe(Compressed_chunks[i],regen_buffer, compressed_chunks_sizes[i], _chunksize*sizeof(T));
		 return((T*)regen_buffer);
	}
	//Return the size of a given compressed chunk
	void Get_compressed_size()
	{
		for(int i=0;i<Num_chunks;i++)
		{
			cout<<"size "<<i<<"= "<<compressed_chunks_sizes[i]<<endl;
		}
	}
	
	size_t Get_compressed_size(int i)
	{
		
	 return(compressed_chunks_sizes[i]);
		
	}
	
	
	char Get_compressed_byoffset(int offset)
	{
		Splitted_Compressed_chunks[offset];
	}
	
	char*join_first()
	{
	char* new_char=new char[compressed_chunks_sizes[0]*sizeof(int)];
	int i=0;
    for(auto x=Splitted_Compressed_chunks.cbegin();x!=Splitted_Compressed_chunks.end();++x)
    {memcpy(new_char+i*Splitted_Compressed_chunks_sizes[0],*x,Splitted_Compressed_chunks_sizes[0]);
    i+=1;
    }
    return new_char;
	}
	
	char* join(int begin, int end,size_t size,vector<const char*>vec,vector<size_t>v)
	{
    char *ch=new char[size];
     for(int i=begin;i<=end;++i)
    {//size_t x=v[i];
    memcpy(ch+((i-begin)*v[i-1]),vec[i],v[i]);
    }
    return ch;
	}
	
	vector<size_t>Get_sizes()
	{return Splitted_Compressed_chunks_sizes ;}
	 size_t get_split_size(int i){
    return Splitted_Compressed_chunks_sizes[i];
    }
};





int main()
{
    unsigned int n=1000000,_chunksize,page_size;
    int * array=new int[n];//First declaration of array of test
 	//Fill the array
	for(size_t i=0;i<n;i++)
    {
		array[i]=rand()%100;
    }
    CompDec<int,200000>A(array,n,4096);
    cout<<"Number of chunks= "<<A.Get_num_chunks()<<endl;
    //A.Compress_chunks();//Compress the chunks
    int num;
	vector<size_t>splitted_sizes=A.Split_compressed(num);
	vector<size_t>splitted=A.Get_sizes();
	/*cout<<"splitted = "<<splitted[8]<<endl;
    cout<<"here = "<<splitted_sizes[39]<<endl;*/
	cout<<"number of compressed pages= "<<num<<endl;
    //A.Decompress_chunks();//Decompress the chunks
    int key;cout<<"Enter key to find ";
    cin>>key;
    
    //cout<<"last term in array "<<array[0]<<endl;
    //cout<<"last Decompressed "<<A.Get_DeCompressed_Chunks()[1][0]<<endl;
    //A.Verify_operation();//Verify compression/decompression operations
    //Find with Full scan
    auto start = std::chrono::high_resolution_clock::now();
    cout<<"offset of "<<key<<" = "<<A.Find(key)<<endl;
    auto stop = std::chrono::high_resolution_clock::now(); 
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start); 
    std::cout << "[SCAN]\t\t Found " <<"first match in " << duration.count() << " μs" << std::endl;
    //Find with zonemaps
    int loc;
    
    cout<<"offset with scan= ";A.Finds(key,true,loc);cout<<loc<<endl;
    
    size_t si;int a,b;
    start = std::chrono::high_resolution_clock::now();
    cout<<"With split "<<A.Find_after(key,true,si,num,a,b)<<endl;
    stop = std::chrono::high_resolution_clock::now(); 
    duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start); 
    std::cout << "[ZoneMaps]\t\t Found " <<"first match in " << duration.count() << " μs" << std::endl;
    cout<<"begin= "<<a<<" end= "<<b<<endl;
    cout<<"size "<<si<<endl;
    /*
    char*ch=new  char[376065*sizeof(char)];ch=A.join();
    cout<<ch[1]<<endl;
    cout<<"len of" <<sizeof(ch)<<endl;
    cout<<"last chunk size "<<A.Get_chunks_sizes()[4]<<endl;*/
    //cout<<"first test "<<strlen(A.Compress_chunk(0))<<endl;//Test to compress just one chunk
    //cout<<A.Decompress_chunk(0)[0];//Decompress just a chunk
    //A.Get_data(5);
    
    
  
    //A.Get_compressed_size();
   /* cout<<"first chunk comp "<<(int)(uint8_t)A.Get_Compressed_Chunks()[0][1]<<endl;
    
    cout<<"first chunk comp "<<(char)(A.Get_Compressed_Chunks()[0][1]+36)<<endl;
		 cout<<"ss"<<float(A.Get_compressed_size(0))<<endl;*/
		 //cout<<(A.get_split(0)).size()<<endl;
		 
		// cout<<A.Get_Compressed_Chunks().size()<<endl;
    	 //char*ch=new  char[A.Get_compressed_size(0)];
    	 //ch=A.join_splitted(0, 1,0);
    	 //cout<<"\n\n\n\n\n"<<endl;
    	 /*if(strcmp(ch,A.Get_Compressed_Chunks()[0])==0)
    	 cout<<"good"<<endl;
    	 else
    	 cout<<"false"<<endl;
    	 cout<<*(ch+10)<<endl;
    	 cout<<"number of pages "<<A.get_size()<<endl;*/
    	 //char *regen_buffer=new  char[n*sizeof(int)];
         //cout<<"a "<<array[19999]<<endl;
		 //LZ4_decompress_safe(A.get_split(94),regen_buffer, A.Get_compressed_size(0), 20000*sizeof(int));
		 //cout<<"\n"<<*((int*)regen_buffer)<<endl;
		 //cout<<"first chunk of the array "<<A.Get_Chunks()[0][0]<<endl;*/
    	 cout<<"first comp size "<<A.get_split_size(1)<<endl;
    	 //cout<<A.get_split(900)<<endl;
    	 /*char* new_char=new char[200000*sizeof(int)];
    	 new_char=A.join_first();
    	 LZ4_decompress_safe(new_char,regen_buffer, A.Get_compressed_size(0), 200000*sizeof(int));
		 cout<<"\n"<<*((int*)regen_buffer+199999)<<endl;*/
		 cout<<"last part of first chunk= "<<array[299999]<<endl;
		 //A.gt_dec();
		 /*
		 char *regen_buffer1=new  char[300000*sizeof(int)];
		 char* new_char1=new char[A.Get_compressed_size(0)];
		 new_char1=A.join(0,7,A.Get_compressed_size(0),A.get_split(),A.Get_sizes());
		 cout<<"spliii"<<splitted[25]<<endl;size_t sz;
		 sz=LZ4_decompress_safe(new_char1,regen_buffer1, A.Get_compressed_size(0), 300000*sizeof(int));
		 cout<<"size decompressed= "<<sz<<endl;
		 cout<<"\n"<<*((int*)regen_buffer1+299999)<<endl;
		 //delete[]new_char1;delete[]regen_buffer1;
		 //vector<size_t>v;/*
		 for(int i=0;i<5;i++)
		 {A.Split_Array(A.Get_Compressed_Chunks()[i],A.Get_compressed_size(i),15001,v);
		 
		 }
		 */
		 /*
		 int counter =0;
		 for(auto x=splitted.cbegin();x!=splitted.cend();++x)
		 {cout<<"part"<<counter<<"="<<*x<<endl;counter++;}
    	 int begin=0;/*
    	 char*ch2=new char[A.Get_compressed_size(0)];
    	 for(int i=begin;i<8;++i)
    	{//size_t x=v[i];
    	memcpy(ch2+((i-begin)*A.Get_sizes()[i]),A.get_split()[i],A.Get_sizes()[i]);
    	}
    	sz=LZ4_decompress_safe(ch2,regen_buffer1, A.Get_compressed_size(0), 200000*sizeof(int));
		 cout<<"size decompressed= "<<sz<<endl;
		 cout<<"\n"<<*((int*)regen_buffer1+199999)<<endl;*/
    return 0;
}
