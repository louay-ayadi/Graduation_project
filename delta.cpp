#include <bits/stdc++.h>
#include <stdio.h>
#include<iostream>
#include <string.h>  // For memcmp()
#include <stdlib.h>  // For exit()
#include <cstdlib>
#include <vector>
#include <chrono> 
#include <cmath>
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
    //LRUCache<int,T*>	*cache;//LRU Cache for decompressed chunks
    shared_ptr<LRUCache<int,T*>> cache;
    int 				index;//Index of element to look for	
    vector<int*>		delta_enc;//vector containing the result of delta_delta encoding	
    int* 				delta;
    vector<unsigned char> compressed;
    vector<int*>		abs;
    vector<char*>	    sign;
    vector<unsigned char> deltas;
    vector<int>			after_shifting;
    int numberofbytes;
    public:
    
    //Constructor
    CompDec(T* ch, size_t ssize)
    {  
    	numberofbytes=0;
    	this->delta=new int[C*sizeof(int)];
    	this->index=0;
    	//this->cache=new LRUCache<int,T*>(100);
    	this->cache=std::make_shared<LRUCache<int,T*>>(100);
    	
    	this->splitted_file_vector=std::make_shared<file_vector<subchunk<subchunk_size>>>("delta_file",true);
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
        encode(this->chunks);
        
        
    }
    
    //template <typename Type>
	void delta_encode(T *buffer, int length){
    	T last = 0;
    	for (int i = 0; i < length; i++){
        	T current = buffer[i];
        	buffer[i] = current - last;
        	last = current;
    	}
	}
	//template <typename Type>
	void delta_decode(T *buffer, int length){
    	T last = 0;
    	for (int i = 0; i < length; i++){
        	T delta = buffer[i];
        	buffer[i] = delta + last;
        	last = buffer[i];
    	}
	}
    
    void encode(vector<int*>_chunks){
    	for(int i=0;i<Num_chunks;i++){
    		delta_encode(_chunks[i],chunks_sizes[i]);
    		delta_encode(_chunks[i],chunks_sizes[i]);
    	}
    	//cout<<"encoded : "<<endl;    	
    }
    
    void copySetBits(unsigned &x, int y,
                 unsigned l, unsigned r)
{
    // l and r must be between 1 to 32
    if (l < 1 || r > 32)
        return ;

    // get the length of the mask
    int maskLength = (1<<(r-l+1)) - 1;

    // Shift the mask to the required position
    // "&" with y to get the set bits at between
    // l ad r in y
    int mask = ((maskLength)<<(l-1)) & y ;
    x = x | mask;
}
    //Add code
void Add_Prefix(int data,vector<unsigned char>& compressed,int&unused,unsigned char& x){
    unsigned c=0;
    int prefix;
    int concerned_bits=0;



        if(data>=-63&&data<=64){
        if(data>0){
            concerned_bits=2;
            prefix=2;
        }
        else if(data<0){
            concerned_bits=3;
            prefix=5;
        }

        }
        else if(data>=-255 &&data<=256){
            if(data>0){
            concerned_bits=3;
            prefix=6;
            }
            else if(data<0){
                concerned_bits=4;
                prefix=13;
            }
        }
        else if(data>=-2047 &&data<=2048){
            if(data>0){
                concerned_bits=4;
                prefix=14;
            }
            else if(data<0){
                concerned_bits=5;
                prefix=29;
            }

        }
        else
        {
            if(data>0){
                concerned_bits=4;
                prefix=15;
            }
            else if (data<0){
                concerned_bits=5;
                prefix=31;
            }
        }

    while(concerned_bits>0){

        if(concerned_bits-unused>0){
        c=0;
        copySetBits(c,prefix,concerned_bits-unused,concerned_bits);
        //cout<<"copied c= "<<c<<endl;
        c=c>>(concerned_bits-unused);
        concerned_bits-=unused;
        unused-=unused;
        }
        else if(concerned_bits-unused<=0){
            c=0;
            copySetBits(c,prefix,1,concerned_bits);
            //cout<<"modified 2c= "<<c<<endl;
            int shift=std::abs(concerned_bits-unused);
            c=c<<shift;
            //cout<<"this case"<<endl;
            //cout<<"modified 3c= "<<c<<endl;
            //cout<<"equal = "<<concerned_bits-1-unused<<endl;
            //cout<<"last byte"<<endl;
            //cout<<"last unused = "<<unused<<endl;
            unused-=concerned_bits;
            //cout<<"last concerned_bits = "<<concerned_bits<<endl;
            //cout<<"last unused = "<<unused<<endl;
            concerned_bits-=concerned_bits;
        }
        /*if(concerned_bits-unused>0)
        c=c>>(concerned_bits-unused);
        else if(concerned_bits-unused<=0)
        {c=c<<abs(concerned_bits-unused);
            cout<<"this case"<<endl;
            cout<<"modified 3c= "<<c<<endl;
            //cout<<"equal = "<<concerned_bits-1-unused<<endl;
        }*/
        x|=c;
        //cout<<"concerned_bits = "<<concerned_bits<<" "<<" unused = "<<unused<<" "<<endl;


        /*if(concerned_bits-unused>0){
            concerned_bits-=unused;
            unused-=unused;

        }
        else if(concerned_bits-unused<=0){
            cout<<"last byte"<<endl;
            //cout<<"last unused = "<<unused<<endl;
            unused-=concerned_bits;
            cout<<"last concerned_bits = "<<concerned_bits<<endl;
            cout<<"last unused = "<<unused<<endl;
            concerned_bits-=concerned_bits;
        }*/

        /*if((unused!=0 and i>=data_o.size()-1)){
        compressed.push_back(x);

        }*/
        if(unused==0){
        compressed.push_back(x);
        c=0;
        x=0;
        unused=8;
        numberofbytes++;
        //cout<<"new byte started"<<endl;
        }

    }


}
	void compress(int data,vector<unsigned char>& compressed,int&unused,unsigned char& x,bool end=false){
    unsigned c=0;
    int concerned_bits=0;


        if(data==0){
            //cout<<"0 unused = "<<unused<<endl;
           concerned_bits=1;
        }
        else if(data>=-63&&data<=64){
            if(data>0){
                concerned_bits=7;
            }
            else if(data<0){
                data=std::abs(data);
                concerned_bits=6;
            }

        }
        else if(data>=-255 &&data<=256){
             if(data>0){
                concerned_bits=9;
            }
            else if(data<0){
                data=std::abs(data);
                concerned_bits=8;
            }

        }
        else if(data>=-2047 &&data<=2048){
         if(data>0){
                concerned_bits=12;
            }
            else if(data<0){
                data=std::abs(data);
                concerned_bits=11;
            }

        }
        else
        { if(data>0){
                concerned_bits=32;
            }
            else if(data<0){
                data=std::abs(data);
                concerned_bits=31;
            }

        }

    while(concerned_bits>0){

        if(concerned_bits-unused>0){
        c=0;
        copySetBits(c,data,concerned_bits-unused,concerned_bits);
        //cout<<"copied c= "<<c<<endl;
        c=c>>(concerned_bits-unused);
        concerned_bits-=unused;
        unused-=unused;
        }
        else if(concerned_bits-unused<=0){
            c=0;
            copySetBits(c,data,1,concerned_bits);
            //cout<<"modified 2c= "<<c<<endl;
            c=c<<std::abs(concerned_bits-unused);
            //cout<<"this case"<<endl;
            //cout<<"modified 3c= "<<c<<endl;
            //cout<<"equal = "<<concerned_bits-1-unused<<endl;
            //cout<<"last byte"<<endl;
            //cout<<"last unused = "<<unused<<endl;
            unused-=concerned_bits;
            //cout<<"last concerned_bits = "<<concerned_bits<<endl;
            //cout<<"last unused = "<<unused<<endl;
            concerned_bits-=concerned_bits;
        }
        /*if(concerned_bits-unused>0)
        c=c>>(concerned_bits-unused);
        else if(concerned_bits-unused<=0)
        {c=c<<abs(concerned_bits-unused);
            cout<<"this case"<<endl;
            cout<<"modified 3c= "<<c<<endl;
            //cout<<"equal = "<<concerned_bits-1-unused<<endl;
        }*/
        x|=c;
        //cout<<"concerned_bits = "<<concerned_bits<<" "<<" unused = "<<unused<<" "<<endl;


        /*if(concerned_bits-unused>0){
            concerned_bits-=unused;
            unused-=unused;

        }
        else if(concerned_bits-unused<=0){
            cout<<"last byte"<<endl;
            //cout<<"last unused = "<<unused<<endl;
            unused-=concerned_bits;
            cout<<"last concerned_bits = "<<concerned_bits<<endl;
            cout<<"last unused = "<<unused<<endl;
            concerned_bits-=concerned_bits;
        }*/

        /*if((unused!=0 and i>=data_o.size()-1)){
        compressed.push_back(x);

        }*/
        if(unused==0){
        compressed.push_back(x);
        numberofbytes++;
        c=0;
        x=0;
        unused=8;
        //cout<<"new byte started"<<endl;
        }
        else if( end){
            compressed.push_back(x);
            numberofbytes++;
        //cout<<"last byte copied"<<endl;
        break;
        }
    }


} 
	void delta_compress(){
		for(int j=0;j<Num_chunks;j++){
			int unused=8;
			this->numberofbytes=0;
			bool end=0;
			unsigned char x=0;
			for(int i=0;i<chunks_sizes[j];i++){
        		if(i==chunks_sizes[j]-1)
        		end=true;
        		if(chunks[j][i]!=0){
            		Add_Prefix(chunks[j][i],compressed,unused,x);
               		compress(chunks[j][i],compressed,unused,x,end);
               		
               		//numberofbytes++;
           		 }
        		else if(chunks[j][i]==0){
        			compress(chunks[j][i],compressed,unused,x,end);
        			//numberofbytes++;
        		}
        		//cout<<"number of byte for chunk "<<j<<"  "<<numberofbytes<<endl;
    		}
    		number_of_pages_foreach_chunk.push_back(numberofbytes);
		}
	}
	
    size_t get_encoded_size(){
    return compressed.size();
    }
    void Get_delta_compressed(){
    	cout<<"After compression we get : ";
    	for(auto x:compressed)
    	cout<<(int)x<<" ";
    	cout<<endl;
    }
    void print_number_ofpages(){
    	cout<<"\nNumber_of_bytes_foreach_chunk : ";
    	for(auto x:number_of_pages_foreach_chunk)
    	cout<<x<<" ";
    }
    void split_delta(){
    	int Num_pages = (this->compressed.size() - 1) / subchunk_size + 1;
     	int index=0;
    	for(int i=0;i<Num_pages;i++){
    		int num=0;
    		subchunk<subchunk_size>s;
    		while(index!=compressed.size()){
    			//cout<<"while num= "<<num<<" index = "<<index<<endl;
    			s.chunk[num]=compressed[index];
    			index++;
    			num++;
    			if(num==subchunk_size or index>=compressed.size()){
    				splitted_file_vector->push_back(s);
    				//cout<<"num= "<<num<<" index = "<<index<<endl;
    				break;
    			}	
    		}
    	}
    	
		cout<<"numpages = "<<Num_pages<<endl;
    }
    void delta_decode(T*tab,size_t length){
    	for(int i=1;i<length;i++){
    		tab[i]=tab[i]+tab[i-1];
    	}
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
    
    
    
    void gt_dec(){for (int i=0;i<Num_chunks;i++)cout<<Decompressed_chunks_sizes[i]<<endl;}   
    
    //copy setbits per byte for decompression
    void copySetBits_Byte(long &x, unsigned char y,int l, int r){
    // l and r must be between 1 to 8
    if (l < 1 || r > 8)
        return ;

    // get the length of the mask
    int maskLength = (1<<(r-l+1)) - 1;

    // Shift the mask to the required position
    // "&" with y to get the set bits at between
    // l ad r in y
    int mask = ((maskLength)<<(l-1)) & y ;
    x = x | mask;
	}
	
	//copy set_bits used with long numbers
	void copySetBits(int &x, long y,
                 const unsigned l,const unsigned r)
	{
    // l and r must be between 1 to 64
    if (l < 1 || r > 64)
        return ;

    // get the length of the mask
    long maskLength = (1<<(r-l+1)) - 1;

    // Shift the mask to the required position
    // "&" with y to get the set bits at between
    // l ad r in y
    long mask = ((maskLength)<<(l-1)) & y ;
    x = x | mask;
	}
	
	//Function to decode the prefix to know how many bits to read
	void read_prefix(int&unused,int &i,vector<unsigned char>deltas,int&concerned_bits,long&reconstructed){
    int index=0;
    //unsigned reconstructed=0;
    while(concerned_bits>0){
        long j=0;
        int l,r;
        if(concerned_bits>=unused){
            l=1;
            r=unused;
        }
        else if(concerned_bits<unused){
            l=unused+1-concerned_bits;
            r=unused;
           }
        
        copySetBits_Byte(j,deltas[i],l,r);
        if(concerned_bits>=unused){
            j=j<<(concerned_bits-unused);
            unused-=unused;
        }
        else if(concerned_bits<unused){
            j=j>>(unused-concerned_bits);
            unused-=(concerned_bits);
        }
        reconstructed|=j;
        concerned_bits-=(r-l+1);
        if(unused==0){
            i++;
            unused=8;
        }
        index++;
        if(concerned_bits==0){
            //cout<<"ended copying "<<endl;
            if(reconstructed<8){
                //cout<<"concerned_bits = 1 "<<endl;
                concerned_bits=1;
                }
            else if(reconstructed>=8 && reconstructed<12){
                //cout<<"concerned_bits = 7"<<endl;
                concerned_bits=9;
            }
            else if(reconstructed>=12 && reconstructed<14){
                //cout<<"concerned_bits = 9"<<endl;
                concerned_bits=12;
            }
            else if(reconstructed>=14 && reconstructed<15){
                //cout<<"concerned_bits = 12"<<endl;
                concerned_bits=16;
            }
            else if(reconstructed==15){
                //cout<<"concerned_bits = 32"<<endl;
                concerned_bits=36;
            }
            //cout<<"reconstructed = "<<reconstructed<<" "<<endl;
            //decompressed.push_back(reconstructed);
            break;
        }
        else if(concerned_bits!=0 &&i==deltas.size()){
            //decompressed.push_back(reconstructed);
            //cout<<"end of vector"<<endl;
            break;
        }
    }
}
	//fonction to decompress numbers
	void dec(int&unused,int &i,vector<unsigned char>deltas,int &concerned_bits,long&reconstructed){
    int index=0;
    //unsigned reconstructed=0;
    while(concerned_bits>0){
        long j=0;
        int l,r;
        if(concerned_bits>=unused){
            l=1;
            r=unused;
        }
        else if(concerned_bits<unused){
            l=unused+1-concerned_bits;
            r=unused;
           }
        
        copySetBits_Byte(j,deltas[i],l,r);
        if(concerned_bits>=unused){
            j=j<<(concerned_bits-unused);
            unused-=unused;
        }
        else if(concerned_bits<unused){
            j=j>>(unused-concerned_bits);
            unused-=(concerned_bits);
        }
        reconstructed|=j;
        concerned_bits-=(r-l+1);
        if(unused==0){
            i++;
            unused=8;
        }
        index++;
        if(concerned_bits==0){
            //cout<<"ended copying "<<endl;
            //cout<<"reconstructed = "<<reconstructed<<" "<<endl;
            //decompressed.push_back(reconstructed);
            break;
        }
        else if(concerned_bits!=0 &&i==deltas.size()){
            //decompressed.push_back(reconstructed);
            //cout<<"end of vector"<<endl;
            break;
        }
    }
}

	
	
    //Decompress chunk i
    void decompress(int num_chunk){
    	if(num_chunk<0 or num_chunk>Num_chunks){
    		cout<<"Invalid chunk number"<<endl;
    		return;
    	}
    	int begin=accumulate(number_of_pages_foreach_chunk.begin(), number_of_pages_foreach_chunk.begin()+num_chunk, 0);
        int end=accumulate(number_of_pages_foreach_chunk.begin(), number_of_pages_foreach_chunk.begin()+num_chunk+1, 0);
        cout<<"begin = "<<begin<<" end = "<<end<<endl;
        int number_of_prec=accumulate(number_of_pages_foreach_chunk.begin(), number_of_pages_foreach_chunk.begin()+num_chunk, 0);
        /*for(int index=0;index<5;index++){
        	deltas.push_back(splitted_file_vector->at(0).chunk[index]);
        	
        }/*
        for(int index=0;index<1751;index++){
        	deltas.push_back(splitted_file_vector->at(1).chunk[index]);	
        }*/
        int count=0;
        int number_of_bytes_to_read=end-begin;
        int num_p=begin/subchunk_size,start=begin%subchunk_size;
        while(count<number_of_bytes_to_read){
        	deltas.push_back(splitted_file_vector->at(num_p).chunk[start]);
        	start++;
        	count++;
        	if(start==2000)
        	num_p++;	
        }
        cout<<"joined and the number is : "<<number_of_bytes_to_read<<endl;
        int i=0;
    	int unused=8;
    	int tmp_index=0;
    	int tmp_unused;
    	int tmp_concerned_bits=0;
    	int number_of_decompressed=0;
    	while(i!=number_of_bytes_to_read && number_of_decompressed<chunks_sizes[num_chunk])
    	{
        int concerned_bits=4;
        long reconstructed=0;
        tmp_unused=unused;
        tmp_index=i;
        //cout<<" i = "<<i<<" unused = "<<unused<<endl;
        read_prefix(tmp_unused,tmp_index,deltas,concerned_bits,reconstructed);
        reconstructed=0;
        tmp_concerned_bits=concerned_bits;
        dec(unused,i,deltas,concerned_bits,reconstructed);
        //cout<<"concerned_bits after dec = "<<tmp_concerned_bits<<endl;
        if(tmp_concerned_bits==1){
            after_shifting.push_back(reconstructed);
        }
        else if(tmp_concerned_bits==9){
            int data=0;
            if(reconstructed>320){
                copySetBits(data,reconstructed,1,6);
                after_shifting.push_back(-data);
            }
            else{
            copySetBits(data,reconstructed,1,7);
            after_shifting.push_back(data);
            }
        }
        else if(tmp_concerned_bits==12){
            int data=0;
            if(reconstructed>3328){
                copySetBits(data,reconstructed,1,8);
                after_shifting.push_back(-data);
            }
            else{
                copySetBits(data,reconstructed,1,9);
                after_shifting.push_back(data);
            }
        }
         else if(tmp_concerned_bits==16){
            int data=0;
            if(reconstructed>59392){
                copySetBits(data,reconstructed,1,11);
                after_shifting.push_back(-data);
            }
            else{
                copySetBits(data,reconstructed,1,12);
                after_shifting.push_back(data);
            }
        }
        else{
            int data=0;
            if(reconstructed>66571993088){
                //2147483647
                long maskLength = 2147483647;
                //cout<<"maskLength = "<<maskLength<<endl;
                // Shift the mask to the required position
                // "&" with y to get the set bits at between
                // l ad r in y
                long mask = (maskLength) & reconstructed ;
                data=data|mask;
                
                //cout<<"data1 = "<<reconstructed<<endl;
                //copySetBits(data,reconstructed,1,32);
                //cout<<"data2 = "<<data<<endl;
                after_shifting.push_back(-data);
            }
            else{
                long maskLength = 4294967295;
                //cout<<"maskLength = "<<maskLength<<endl;
                // Shift the mask to the required position
                // "&" with y to get the set bits at between
                // l ad r in y
                long mask = (maskLength) & reconstructed ;
                data=data|mask;
                
                //cout<<"data1 = "<<reconstructed<<endl;
                //copySetBits(data,reconstructed,1,32);
                //cout<<"data2 = "<<data<<endl;
               
                after_shifting.push_back(data);
            	}
        	}
        number_of_decompressed++;
        tmp_concerned_bits=concerned_bits;
    	}
    	for(int k=0;k<15;k++)
    	cout<<after_shifting[k]<<" ";
        cout<<endl;
        cout<<"number of decompressed = "<<number_of_decompressed<<" i = "<<i<<endl; ;
    }
    int get_number(int i){
    	return after_shifting.size();
    }
    
    //Destructor
    ~CompDec()
    {     /*      
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
     delete[] compressed_joined;*/
     //delete cache;
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
      /****************** Fill Last chunk **************/
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
    

    
     
   
};


    template<typename T>
	ZoneMapSet<T>createzonemaps(T *array,size_t sizeofzone,size_t total_size){
		ZoneMapSet<T>zonemaps(sizeofzone*sizeof(T),total_size);
        zonemaps.InitFromData(array, total_size);
        return zonemaps;
	}



int main()
{
    system("rm delta_file");
    system("rm test_delta");
    srand(1234);
    unsigned int n=100000,_chunksize,page_size;
    cout<<"Size of array "<<n<<endl;
    file_vector<int> vector_test_queries("test_delta", file_vector<int>::create_file);
    int * array=new int[n];//First declaration of array of test
 	//Fill the array
	for(size_t i=0;i<n;i++)
    {
		array[i]=1;
		vector_test_queries.push_back(array[i]);
		//cout<<array[i]<<" ";
    }
    cout<<endl;
    
    CompDec<int,300,20>A(array,n); 
   // A.encode();   
    //cout<<"encoded : "<<endl;
    /*for(int i=4;i<8;i++){
    	for(int j=0;j<5;j++)
    		cout<<abs(A.get_encoded()[i][j])<<" ";
    	}
    int x=-8;
    cout<<abs(x);*/
    /*
    for(int i=0;i<4;i++){
    	for(int j=0;j<5;j++)
    	cout<<A.Get_Chunks()[i][j]<<" ";
    }*/
    A.delta_compress();
    A.print_number_ofpages();
    cout<<"\n size of compressed = "<<A.get_encoded_size()<<endl;
    //A.Get_delta_compressed();
    A.split_delta();
    A.decompress(1);
    cout<<"last number to check "<<A.get_number(29999)<<endl;
    return 0;
    }

