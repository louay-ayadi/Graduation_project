#include <bits/stdc++.h>
#include <stdio.h>
#include<iostream>
#include <string.h>  // For memcmp()
#include <stdlib.h>  // For exit()
#include <cstdlib>
#include <vector>
#include <chrono> 
#include "lz4.h" 
#include "/home/louay/Downloads/File-Vector-master/file_vector.hpp"
#include "LRUCache.cpp"
using namespace std;

template<size_t s>
struct subchunk{
char chunk[s];
};

template<typename T,int C,size_t subchunk_size>
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
    vector<size_t>	 	 Splitted_Compressed_chunks_sizes;//Vector of Splitted compressed chunks' sizes
    int					 page_size=subchunk_size;
    ZoneMapSet<T>		 zonemaps;
    vector<const char *> vec;
    std::shared_ptr<file_vector<subchunk<subchunk_size>>> splitted_file_vector;//File_vector for compressed pages
    char*				 decompressed_data;
    char* 				 compressed_joined;
    size_t 				 max_compressed_size;
	vector<int> 		 number_of_pages_foreach_chunk;//Vector containing number of pages foreach compressed chunk
    LRUCache<int,T*>	*cache;//LRU Cache for decompressed chunks
    int 				index;//Index of element to look for		
    public:
    
    //Constructor
    CompDec(T* ch, size_t ssize)
    {  
    	this->index=0;
    	this->cache=new LRUCache<int,T*>(100);
    	this->splitted_file_vector=std::make_shared<file_vector<subchunk<subchunk_size>>>("splitted",true);
    	size=ssize;
        if(ssize%_chunksize==0)
            	{Num_chunks=ssize/_chunksize;}
        else if (ssize%_chunksize!=0)
            	{Num_chunks=(ssize/_chunksize)+1;}
            	cout<<"num_chunks "<<this->Num_chunks<<endl;
        this->chunks=Split_Array(ch,ssize,_chunksize);
        //Create zone_maps
        ZoneMapSet<T>zonemapss(C*sizeof(T),ssize);
        this->zonemaps=zonemapss;
        zonemaps.InitFromData(ch, ssize);
        auto start = std::chrono::high_resolution_clock::now();     
        Compress_chunks(this->chunks,this->Num_chunks,this->size);
        auto stop = std::chrono::high_resolution_clock::now(); 
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start); 
        std::cout << "compression took " <<duration.count() << " μs" << std::endl;
        auto it = max_element(std::begin(compressed_chunks_sizes), std::end(compressed_chunks_sizes));
        //cout<<"max compressed size"<<*it<<endl;
        /*start = std::chrono::high_resolution_clock::now(); 
        Decompress_chunks();
        stop = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start); 
        std::cout << "Decompression took " <<duration.count() << " μs" << std::endl;*/
        Split_compressed(total_pages,0);
  		this->decompressed_data=new  char[chunks_sizes[0]*sizeof(int)];
    	
    	this->max_compressed_size=*it;
		this->compressed_joined=new char[max_compressed_size];
		
		for(int i=0;i<Num_chunks;i++)
		{
		 int Num_p = (compressed_chunks_sizes[i] - 1) / subchunk_size + 1;
		 number_of_pages_foreach_chunk.push_back(Num_p);
		}
		//for(int i=0;i<Num_chunks;i++)
		//cout<<"comp "<<i<<"has "<<number_of_pages_foreach_chunk[i]<<"pages"<<endl;
		cout << "Sum = "<< accumulate(number_of_pages_foreach_chunk.begin(), number_of_pages_foreach_chunk.end(), 0)<<endl; 
    }
    std::shared_ptr<file_vector<subchunk<subchunk_size>>> Get_file()
    {
    return splitted_file_vector;
    }
    int Get_total_pages_number()
    {
    return total_pages;
    }
    int Get_num_chunks()
    {return Num_chunks;}
    
    
    //Compress chunks Method
    void Compress_chunks(vector<T*> chunks,int Num_chunks,int size)
    { for(int i=0;i<Num_chunks-1;i++)
   		    {//Allocate memory
    		char *src=new char[C*sizeof(T)];
    		//Copy given string into allocated memory
			memcpy(src,chunks[i],_chunksize*sizeof(T));
			//Max size for compression
			
			const int max_dst_size = LZ4_compressBound(_chunksize*sizeof(T));
		
			//Allocate memory for compressed chunk
    		char *dest = new char[(size_t)max_dst_size];
			if(dest==NULL)
				run_screaming("Failed to allocate memory for *compressed_data.", 1);
			const int compressed_data_size = LZ4_compress_default(src, dest, _chunksize*sizeof(T), max_dst_size);
			//const int compressed_data_size =LZ4_compress_fast(src, dest, _chunksize*sizeof(T), max_dst_size,3);
			compressed_chunks_sizes.emplace_back(compressed_data_size);
			Compressed_chunks.emplace_back(dest);	
	     	}
	     	
	     	/*******************************************************/
			//Last Chunk compression
			char *last_src=new char[(size-(_chunksize*(Num_chunks-1)))*sizeof(T)];
			
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
                end=total_pages;
            }
            //to here
       join(compressed_joined,begin,end-1,compressed_chunks_sizes[i],Splitted_Compressed_chunks,Splitted_Compressed_chunks_sizes);
		 	sz=LZ4_decompress_safe(compressed_joined,decompressed_data, compressed_chunks_sizes[i], chunks_sizes[i]*sizeof(int));
            
            
            
            	T* decompressed_chunk=(T*)decompressed_data;
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
    size_t Find_with_file_vector(T key, bool use_zone_maps,size_t &sz,int total_pages,int&begin,int&end,bool &found_it)
    { for(size_t i=0; i<zonemaps.Size(); i++) {
    	auto zm = zonemaps.Get(i);
        if(zm->Intersects(key, key)) {//cout<<"intersect and key= "<<i<<endl;
           	int num_pages=(compressed_chunks_sizes[i] - 1) / page_size + 1;       
            begin=accumulate(number_of_pages_foreach_chunk.begin(), number_of_pages_foreach_chunk.begin()+i, 0);
            end=accumulate(number_of_pages_foreach_chunk.begin(), number_of_pages_foreach_chunk.begin()+i+1, 0);
            join(compressed_joined,begin,end-1,compressed_chunks_sizes[i],splitted_file_vector,Splitted_Compressed_chunks_sizes);
		 	//auto start = std::chrono::high_resolution_clock::now(); 
		 	sz=LZ4_decompress_safe(compressed_joined,decompressed_data, compressed_chunks_sizes[i], chunks_sizes[i]*sizeof(int));
          	/*auto stop = std::chrono::high_resolution_clock::now(); 
    		auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
   			 if(duration.count()>0) 
    		{std::cout << "Decompression(fv_no_cache) took " <<duration.count() << " μs" << std::endl;
    		cout<<"chunk"<<i<<endl;
    		}*/
          	T* decompressed_chunk=(T*)decompressed_data;
                for(size_t j=zm->start_loc; j<zm->end_loc; j++) {
                    if(decompressed_chunk[j-(i*_chunksize)] == key) {//cout<<"intersect and key= "<<key <<" "<<i<<" "<<j<<endl;
                        
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
    //find fv+zm+cache_impl
    size_t Find_with_file_vector_cache(T key, bool use_zone_maps,int total_pages,bool &found_it)
    { for(size_t i=0; i<zonemaps.Size(); i++) {
    	auto zm = zonemaps.Get(i);
        if(zm->Intersects(key, key)) {
          if(cache->exist(i)){//auto start = std::chrono::high_resolution_clock::now();
            for(size_t j=zm->start_loc; j<zm->end_loc; j++) {
                    if(cache->get(i)[j-(i*_chunksize)] == key) {//cout<<cache->get(i)[j-(i*_chunksize)]<<" "<<j<<" "<<i<<"start "<<zm->start_loc<<" zm->end_loc"<<zm->end_loc<<endl;
                    //cout<<"test "<<cache->get(4)[18679]<<endl;
                        found_it = true;
                        /*auto stop = std::chrono::high_resolution_clock::now();
     					auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start); 
    					std::cout << "[time for checking] " << (float)duration.count()<< " μs" << std::endl;*/
                        return j; 
                        break;
                    }
                }
    		}
          else{//cout<<"not existing "<<key<<endl;
           	int num_pages=0;
            if(compressed_chunks_sizes[i]%page_size==0)
            num_pages=compressed_chunks_sizes[i]/page_size;
            else 
            num_pages=(compressed_chunks_sizes[i]/page_size)+1;         
            int begin=accumulate(number_of_pages_foreach_chunk.begin(), number_of_pages_foreach_chunk.begin()+i, 0);
            int end=accumulate(number_of_pages_foreach_chunk.begin(), number_of_pages_foreach_chunk.begin()+i+1, 0);
            join(compressed_joined,begin,end-1,compressed_chunks_sizes[i],splitted_file_vector,Splitted_Compressed_chunks_sizes);
            LZ4_decompress_safe(compressed_joined,decompressed_data, compressed_chunks_sizes[i], chunks_sizes[i]*sizeof(int));
	       	T* decompressed_chunk=(T*)decompressed_data;
          	for(size_t j=zm->start_loc; j<zm->end_loc; j++) {
                    if(decompressed_chunk[j-(i*_chunksize)] == key) {//cout<<"exists and not in cache "<<j<<" "<<key <<endl;
                    	cache->put(i,decompressed_chunk);             
                        found_it = true;
                        return j; 
                        break;
                    }
                }
              }
            }
           
            if (found_it)
            break;
         }
    }
    
    
    //add a new function to find a query in the file vector without the zonemaps
    
    bool Find_with(T key, bool use_zone_maps,size_t &sz,int total_pages)
    {
    bool found_it = false;
    //if(use_zone_maps)
    int i=0;
    while(!found_it) {
    	
            int num_pages;
            if(compressed_chunks_sizes[i]%page_size==0)
            num_pages=compressed_chunks_sizes[i]/page_size;
            else
            num_pages=(compressed_chunks_sizes[i]/page_size)+1;
            //added here
            int begin,end;
            /*if(i!=Num_chunks-1){
                begin=i*num_pages;
                end=(i+1)*num_pages;
            }
            else{
            	begin=(Num_chunks-1)*num_pages;
                end=total_pages;
            } 	*/
            begin=accumulate(number_of_pages_foreach_chunk.begin(), number_of_pages_foreach_chunk.begin()+i, 0);
            end=accumulate(number_of_pages_foreach_chunk.begin(), number_of_pages_foreach_chunk.begin()+i+1, 0);
            //cout<<"begin= "<<begin<<endl;cout<<"end= "<<end<<endl;
		 	join(compressed_joined,begin,end-1,compressed_chunks_sizes[i],splitted_file_vector,Splitted_Compressed_chunks_sizes);
		 	sz=LZ4_decompress_safe(compressed_joined,decompressed_data, compressed_chunks_sizes[i], chunks_sizes[i]*sizeof(int));
		 	//cout<<"ratio= "<<(float) compressed_chunks_sizes[i]/sz<<endl;
		 	//cout<<"i= "<<i<<endl;
		 	//cout<<"sz"<<sz<<endl;
            T* decompressed_chunk=(T*)decompressed_data;
                for(int j=0;j<chunks_sizes[i]; j++) {
                    if(decompressed_chunk[j] == key) {
                        found_it = true;
                        break;
                    }
                    //cout<<"not found"<<decompressed_chunk[j]<<endl;
                }
			i++;
            if (i==Num_chunks or found_it)
            break;
            
           
         }
         
	return found_it; 
    }
    
    
    
    
    
    //Here it ends
    size_t Find_with_file_vector_without_zonemaps(T key, bool use_zone_maps,size_t &sz,int total_pages,int&begin,int&end)
    {
    bool found_it = false;
    begin=0;
    end=total_pages;
    char *regen_buffer1=new  char[size*sizeof(int)];
    size_t total_compressed_size=0;
    for(int i=0;i<Num_chunks-1;i++)
    total_compressed_size+=compressed_chunks_sizes[i];
	char* new_char1=new char[total_compressed_size];
	new_char1=join(begin,end-1,total_compressed_size,splitted_file_vector,Splitted_Compressed_chunks_sizes);
	sz=LZ4_decompress_safe(new_char1,regen_buffer1, total_compressed_size, size*sizeof(int));
    T* decompressed_chunk=(T*)regen_buffer1;
                for(size_t j=0; j<size; j++) {
                    if(decompressed_chunk[j] == key) {
                        
                        found_it = true;
                        return j; 
                        break;
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
     chunks_sizes.clear();
     Splitted_Compressed_chunks.clear();
     number_of_pages_foreach_chunk.clear();
     delete[] decompressed_data;
     delete[] compressed_joined;
     delete cache;
    }
    
    //Function to Split the Array to little chunks depending on given _chunksize
    vector<T*> Split_Array(const T* Tab,size_t n, int _chunksize)
    {
     vector<T*>chunks;
     
     int Num_chunks = (n - 1) / _chunksize + 1;
     //Num_chunks=n/_chunksize;
     
     for (int i=0; i <Num_chunks-1; i++) //chunks=n/_chunksize
         {  
          T *new_chunk=new T[_chunksize];
          for (int j=0; j<_chunksize; j++) 
              { 
               new_chunk[j]=Tab[j+i*_chunksize];
              }
               chunks.emplace_back(new_chunk);
          	   chunks_sizes.emplace_back(_chunksize);
         }
      /******************Fill Last chunk **************/
      T *last_chunk=new T[n-(_chunksize*(Num_chunks-1))];
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
	vector<size_t>Split_compressed(int &num,int begin)
	{   num=0;
		for(int i=begin;i<Num_chunks;i++)
		{
		 vector<const char*>compressed;
		  vector<size_t>sizes;
		  int index=0;
		  compressed=Split_Array(Compressed_chunks[i],(compressed_chunks_sizes[i]),page_size,sizes);
		  for (auto j = compressed.begin(); j != compressed.end(); ++j) 
         	{Splitted_Compressed_chunks.emplace_back(*j);
		  	 num+=1;
		  	 
		  	 subchunk<subchunk_size>s ;//ch.chunk=new char[sizes[index]];
		  	 memcpy(s.chunk,*j,sizes[index]);
			 splitted_file_vector->push_back(s);
			 //vec.emplace_back(s);
			 index++;
			}//cout<<"number of pages"<<(sizes.size())*Num_chunks<<endl;
			for (auto j = sizes.begin(); j != sizes.end(); ++j) 
         	{Splitted_Compressed_chunks_sizes.emplace_back(*j);
		  	}
			         
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
	}/*
	const char*get_split(int i)
	{
	return(Splitted_Compressed_chunks[i]);
	}*/
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
	
	void join(char*dest,int begin, int end,size_t size,vector<const char*>vec,vector<size_t>v)
	{for(int i=begin;i<=end;++i)
    	{
    	memcpy(dest+((i-begin)*v[i-1]),vec[i],v[i]);
    	}
    }
	//Join function with file vector containing splitted compressed chunks
	void join(char* dest,int begin, int end,size_t size,std::shared_ptr<file_vector<subchunk<subchunk_size>>> vec,vector<size_t>v)
	{
    for(int i=begin;i<=end;++i)
    	{
    	memcpy(dest+((i-begin)*v[i-1]),vec->at(i).chunk,v[i]);
    	
    	}
    }
	
	vector<size_t>Get_sizes()
	{return Splitted_Compressed_chunks_sizes ;}
	 size_t get_split_size(int i){
    return Splitted_Compressed_chunks_sizes[i];
    }
    //overload of the operator [] to access an elmt at a given index, decompress the concerned chunk and return it
     const T& operator[](int index) 
	{this->index=index; 
    if (index < 0 || index >= size) { 
        cout << "Array index out of bound, exiting...";
        exit(0); 
    }
    int chunk_number = (index ) / _chunksize ;
    if(cache->exist(chunk_number))
    //cout<<"begin "<<chunk_number<<endl;
    {//cout<<"found"<<endl;
    return cache->get(chunk_number)[index-(chunk_number*_chunksize)]; }
    else 
    {//cout<<"not found"<<endl;
    int begin=accumulate(number_of_pages_foreach_chunk.begin(), number_of_pages_foreach_chunk.begin()+chunk_number, 0);
    int end=accumulate(number_of_pages_foreach_chunk.begin(), number_of_pages_foreach_chunk.begin()+chunk_number+1, 0);
    cout<<"begin= "<<begin<<endl;cout<<"end= "<<end<<endl;
	join(compressed_joined,begin,end-1,compressed_chunks_sizes[chunk_number],splitted_file_vector,Splitted_Compressed_chunks_sizes);
    LZ4_decompress_safe(compressed_joined,decompressed_data, compressed_chunks_sizes[chunk_number], chunks_sizes[chunk_number]*sizeof(int));
    T* decompressed_chunk=(T*)decompressed_data;
    //cout<<"dec "<<decompressed_chunk[index-(chunk_number*_chunksize)];
    cache->put(chunk_number,decompressed_chunk);
    return decompressed_chunk[index-(chunk_number*_chunksize)]; 
	}
	}
	
	//Definition of the operator at() to access an elmt at a given index, decompress the concerned chunk and return it
	inline const T& at(size_t const index){
        if (index < 0 || index >= size) {
            throw out_of_range("compression_class::at(index)");
        } 
    int chunk_number = (index ) / _chunksize ;  
    int begin=accumulate(number_of_pages_foreach_chunk.begin(), number_of_pages_foreach_chunk.begin()+chunk_number, 0);
    int end=accumulate(number_of_pages_foreach_chunk.begin(), number_of_pages_foreach_chunk.begin()+chunk_number+1, 0);
    join(compressed_joined,begin,end-1,compressed_chunks_sizes[chunk_number],splitted_file_vector,Splitted_Compressed_chunks_sizes);
    //auto start = std::chrono::high_resolution_clock::now(); 
    LZ4_decompress_safe(compressed_joined,decompressed_data, compressed_chunks_sizes[chunk_number], chunks_sizes[chunk_number]*sizeof(int));
    /*auto stop = std::chrono::high_resolution_clock::now(); 
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
    if(duration.count()>5) 
    {std::cout << "Decompression took " <<duration.count() << " μs" << std::endl;
    cout<<"chunk"<<chunk_number<<endl;
    }*/
    T* decompressed_chunk=(T*)decompressed_data;
    return decompressed_chunk[index-(chunk_number*_chunksize)]; 
    }
    
    const T* begin()
    {
     index=0;
     return &at(index);
    } 
    const T* end()
    {
     index=size-1;
     return &at(index);
    }/*
    const T& operator++ ()
    { //return *at(index);
    }*/
    const T& operator ++()
    {
     /*if(index%_chunksize ==1&&index>1)*/
     ++index;
     return(at(index));
         
     //T* decompressed_chunk=(T*)decompressed_data;
     //return *(decompressed_chunk+index);
    }
void test()
    {
    //this->decompressed_data=decompressed_data;
     T* decompressed_chunk=(T*)decompressed_data;
     cout<<""<<*(decompressed_chunk+index+1)<<endl;
    }
    
     /*operator vector<char*>() {
        vector<char*> dst;
        dst.assign(splitted_file_vector.begin(), splitted_file_vector.end());
        return dst;
    }*/
    void push_back(T*array,size_t n)
    {
     this->size+=n;
     int Num_chunks = (n - 1) / _chunksize + 1;
     this->Num_chunks+=Num_chunks;
     vector<T*>chunks=Split_Array(array,n,this->_chunksize);
     for(auto x=chunks.begin();x!=chunks.end();x++)
     this->chunks.emplace_back(*x);
     ZoneMapSet<T>zone_maps(C*sizeof(T),n);
     zone_maps.InitFromData(array,n);
     for(uint64_t i=0; i<zone_maps.Size(); i++)
        this->zonemaps.zone_maps.push_back(zone_maps.zone_maps[i]);
     cout<<"zonemaps "<<zone_maps.Size()<<endl;
     cout<<"Total zonemaps "<<this->zonemaps.Size()<<endl;
     cout<<"total "<<this->chunks.size()<<endl;
     Compress_chunks(chunks,Num_chunks,n);
     int num_p;
     Split_compressed(num_p,this->Num_chunks-Num_chunks);
     cout<<"num_p "<<num_p<<endl;
     this->total_pages+=num_p;
     for(int i=this->Num_chunks-Num_chunks;i<this->Num_chunks;i++)
		{
		 int Numb_of_pages = (compressed_chunks_sizes[i] - 1) / subchunk_size + 1;
		 number_of_pages_foreach_chunk.push_back(Numb_of_pages);
		}
     
    }
};


    template<typename T>
	ZoneMapSet<T>createzonemaps(T *array,size_t sizeofzone,size_t total_size){
		ZoneMapSet<T>zonemaps(sizeofzone*sizeof(T),total_size);
        zonemaps.InitFromData(array, total_size);
        return zonemaps;
	}



int main()
{
    system("rm splitted");
    system("rm test_queries");
    srand(1234);
    unsigned int n=20000000,_chunksize,page_size;
    cout<<"Size of array "<<n<<endl;
    file_vector<int> vector_test_queries("test_queries", file_vector<int>::create_file);
    int * array=new int[n];//First declaration of array of test
 	//Fill the array
	for(size_t i=0;i<n;i++)
    {
		array[i]=rand()%10000;
		vector_test_queries.push_back(array[i]);
    }
    //Some keys to search for 
    vector<int> queries = vector<int>({0, 31, 500, 500,0, 55, 550, 38,9, 9, 50, 678,0, 31, 500, 638,0, 31, 500, 670,0, 31, 500, 675,0, 31, 500, 677,0, 31, 500, 538,0, 31, 500, 758,0, 31, 500, 6000,0, 31, 50, 678,0, 31, 500, 538,0, 31, 500, 538,0, 31, 500, 538,0, 31, 500, 538,0, 31, 500, 638,0, 31, 500, 678,0, 31, 500, 738,0, 310, 555, 538,0, 31, 421, 677,600, 311, 525, 538,0, 31, 500, 678,789, 310, 700, 538,0, 31, 500, 638,0, 31, 55, 5300});
    CompDec<int,16000,4096>A(array,n);    vector<int>indexes=vector<int>({3783699,98680,248273,248273,3783699,1474053,1071008,266660,541594,541594,4253954,1499462,3783699,98680,248273,45032,3783699,98680,211,132,0,100000,25,11,30,2,3,40000,12,15,20,16,10,25,24,110,111,101,211,132,0, 100000,25,11,30,2,3,50000,12,15,20,16,10,25,24,110,111,101,211,132,0,100000,25,11,30,2,3,60000,12,15,20,16,10,25,24,110,111,101,211,132,0, 100000,25,11,30,2,3,70000,12,15,20,16,10,25,24,110,111,101,211,132,0,100000,25,11,30,2,3,70000,12,15,20,16,10,25,24,110,111,101,211,132,0, 100000,25,11,30,2,3,700000,12,150000,20,16,10,25,24,110000,111,101,211,132});
    int index;
    cout<<"give the index of the element you want to access ";
    cin>>index;
    cout<<"verify here "<<array[19999999]<<endl;
    
    //cout<<"array[20000] "<<array[20000]<<endl;cout<<"array[20001] "<<array[20001]<<endl;
    //cout<<"array[20002] "<<array[20002]<<endl;cout<<"array[20003] "<<array[20003]<<endl;
    
    //search with cache
    auto start = std::chrono::high_resolution_clock::now(); 
    for(auto &index:indexes)
    A.at(index);
    auto stop = std::chrono::high_resolution_clock::now(); 
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start); 
    std::cout << "avg search with cache took " <<(float)(duration.count())/indexes.size() << " μs" << std::endl;
    //search with warm cache
    start = std::chrono::high_resolution_clock::now(); 
    for(auto &index:indexes)
    A.at(index);
    stop = std::chrono::high_resolution_clock::now(); 
    duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start); 
    std::cout << "avg search with warm cache took " <<(float)(duration.count())/indexes.size() << " μs" << std::endl;
    
    //search without cache
    start = std::chrono::high_resolution_clock::now(); 
    for(auto &index:indexes)
    A.at(index);
    stop = std::chrono::high_resolution_clock::now(); 
    duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start); 
    std::cout << "avg search without cache took " <<(float)(duration.count())/indexes.size() << " μs" << std::endl;
    /**/
    
    start = std::chrono::high_resolution_clock::now(); 
    for(auto &index:indexes)
    	vector_test_queries.at(index);
    stop = std::chrono::high_resolution_clock::now(); 
    duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start); 
    std::cout << "avg search without compression " <<(float)(duration.count())/indexes.size()<< " μs" << std::endl;
    //cout<<"begin = "<<*A.begin()<<endl;
    cout<<"Number of chunks= "<<A.Get_num_chunks()<<endl;
    vector<size_t>splitted=A.Get_sizes();int nump=0;
	/*for(auto i=splitted.begin();i!=splitted.end();i++)
	{nump++;
	//if(*i>32)
	//cout<<"page "<<*i<<endl;
	}*/
	//cout<<"number of compressed pages= "<<nump<<endl;
	cout<<"number of compressed pages= "<<A.Get_total_pages_number()<<endl;
	//cout<<"size 19 "<<A.Get_compressed_size(19)<<endl;
    
    int coun=0;
    int found = 0;
	/*SCAN of real file vector*/
    start = std::chrono::high_resolution_clock::now(); 
    
    for(auto &query : queries) {
        for(auto &v : vector_test_queries) {
            if(v == query) {
                found++;
                break;
            }
        }
    }
    stop = std::chrono::high_resolution_clock::now(); 
    duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start); 
    std::cout << "[avg SCAN of real file vector]\t\t Found " << found << " matches in " << (float)(duration.count())/queries.size() << " μs" << std::endl;
    /*SCAN of the compressed file_vector*
    start = std::chrono::high_resolution_clock::now(); 
	for(auto &query : queries)
    {
    	size_t si;
    	if(A.Find_with(query,true,si,A.Get_total_pages_number()))
    		{coun++;} 
    }
    stop = std::chrono::high_resolution_clock::now(); 
    duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start); 
    std::cout << "[SCAN of the compressed file_vector]\t\t Found " << coun << " matches in " << float(duration.count()/queries.size()) << " μs" << std::endl;
    */
   
    //Find with Full scan of the real vector
    found=0;
    
     start = std::chrono::high_resolution_clock::now(); 
    
    for(auto &query : queries) {
        for(int i=0;i<n;i++) {
            if(array[i] == query) {
                found++;
                break;
            }
        }
    }
    stop = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start); 
    std::cout << "[avg Full scan of the vector]\t\t Found " << found << " matches in " << (float)duration.count()/queries.size() << " μs" << std::endl;
    auto zonemaps =createzonemaps<int>(array,20000,n);
    //Find with Zonemaps with the real vector
    found = 0;
    start = std::chrono::high_resolution_clock::now(); 
    
    for(auto &query : queries) {
        bool found_it = false;
        for(size_t i=0; i<zonemaps.Size(); i++) {
            auto zm = zonemaps.Get(i);
            if(zm->Intersects(query, query)) {
                for(size_t j=zm->start_loc; j<zm->end_loc; j++) {
                    if(vector_test_queries.at(j) == query) {//cout<<i<<" "<<j<<" "<<query<<endl;
                        found++;
                        found_it = true;
                        break;
                    }
                }
            }
            if(found_it)
                break;
        }
    }
    stop = std::chrono::high_resolution_clock::now(); 
    duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start); 
    std::cout << "[avg ZONEMAPS]\t Found " << found << " matches in " << (float)duration.count()/queries.size() << " μs" << std::endl;
    bool found_it;found=0;
    /*ZoneMaps +Compressed File_vector*/
     start = std::chrono::high_resolution_clock::now();
    for(auto &query : queries)
    {	found_it=false;
    	size_t si;int a,b;
    	A.Find_with_file_vector_cache(query,true,A.Get_total_pages_number(),found_it);
		if(found_it)
			found++;
    //cout<<"begin= "<<a<<" end= "<<b<<endl;
    //cout<<"size "<<si<<endl;
    }
     stop = std::chrono::high_resolution_clock::now();
     duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start); 
    std::cout << "[avg ZoneMaps +Compressed File_vector]\t\t Found "<<found<<" matches in\t" << (float)duration.count()/queries.size() << " μs" << std::endl;
    
    
    found=0;
    /*ZoneMaps +Compressed File_vector+warm cache*/
     start = std::chrono::high_resolution_clock::now();
    for(auto &query : queries)
    {	found_it=false;
    	size_t si;int a,b;
    	A.Find_with_file_vector_cache(query,true,A.Get_total_pages_number(),found_it);
		if(found_it)
			found++;
    //cout<<"begin= "<<a<<" end= "<<b<<endl;
    //cout<<"size "<<si<<endl;
    }
     stop = std::chrono::high_resolution_clock::now();
     duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start); 
    std::cout << "[avg ZoneMaps +Compressed File_vector+warm cache]\t\t Found "<<found<<" matches in\t" << (float)duration.count()/queries.size() << " μs" << std::endl;
    
     found=0;
    /*ZoneMaps +Compressed File_vector+warm cache*/
     start = std::chrono::high_resolution_clock::now();
    for(auto &query : queries)
    {	found_it=false;
    	size_t sz;int a,b;
    	A.Find_with_file_vector(query,true,sz,A.Get_total_pages_number(),a,b,found_it);
		if(found_it)
			found++;
    //cout<<"begin= "<<a<<" end= "<<b<<endl;
    //cout<<"size "<<si<<endl;
    }
     stop = std::chrono::high_resolution_clock::now();
     duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start); 
    std::cout << "[avg ZoneMaps +Compressed File_vector+ without cache]\t\t Found "<<found<<" matches in\t" << (float)duration.count()/queries.size() << " μs" << std::endl;
    //A.print_ratio();
    /*int test=0;
    /*
    cout<<"end "<<*A.end()<<endl;
    cout<<"verif "<<A[99999]<<endl;
    for(auto x=A.begin();x!=A.end();x++)
    {test++;
    if(*x<0)
    cout<<*x<<endl;}
    cout<<"number "<<test<<endl;
    test=0;
    for(auto &v:A)
    test++;
    cout<<"number "<<test<<endl;
    //cout<<v<<endl;*/
    A.push_back(array,n);
    cout<<"Number of chunks= "<<A.Get_num_chunks()<<endl;
    cout<<"verify "<<A[39999999]<<endl;
    //cout<<"verify "<<array[10000000]<<endl;
    return 0;
    }

