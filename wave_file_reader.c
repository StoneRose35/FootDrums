
#include <stdio.h>
#include <stdlib.h>
#include <cstring>

#include "wave_file_reader.h"



/*
int main(int argc, char** argv)
{
	wav_header * hdr;
	wav_format * w_fmt;
	data_header * d_hdr;
	sample s_l,s_r;
	sample * sample_array;
	
	unsigned short bytes_p_channel;
	FILE *f;

	if (argc != 2)
	{
		printf("call as follows: %s <pathToWavFile>\n",argv[0]);
		return 0;
	}
	int sample_size;
	read_wav_to_array(argv[1],sample_array,&sample_size);
	
}
*/

int read_wav_to_array(char * fname, sample ** sample_array,int * array_size)
{
		wav_header * hdr;
	wav_format * w_fmt;
	data_header * d_hdr;
	sample s_l,s_r;
	
	unsigned short bytes_p_channel;
	FILE *f;


	hdr = (wav_header*)malloc(sizeof(wav_header));
	w_fmt = (wav_format*)malloc(sizeof(wav_format));
	d_hdr = (data_header*)malloc(sizeof(data_header));
	
	f=fopen(fname,"rb");
	
	fread(hdr,sizeof(wav_header),1,f);
	if (strncmp(hdr->riff,"RIFF",4)!=0)
	{
		printf("header is %s, not a valid Wave file\n",hdr->riff);
		return -1;
		
	}	
	
	if(strncmp(hdr->wave,"WAVE",4)!=0)
	{
		printf("What should be WAVE in header is %s\n",hdr->wave);
		return -1;
	}
	
	fread(w_fmt,sizeof(wav_format),1,f);
	
	if(strncmp(w_fmt->id,"fmt ",4)!=0)
	{
		printf("What should be fmt in format header is %s\n",w_fmt->id);
		return -1;
	}
	
/*
	printf("format tag is %hu\n",w_fmt->format_tag);
	printf("number of channels are %hu\n",w_fmt->n_channels);	
	printf("sample rate is %u\n",w_fmt->sample_rate);
	printf("data rate is %u\n",w_fmt->data_rate);
	printf("frame size is %hu\n",w_fmt->frame_size);
*/	
	if (w_fmt->format_tag != 1)
	{
		printf("wave file is not a pcm file, cannot read it\n");
		return -1;
	}


	bytes_p_channel=w_fmt->frame_size / w_fmt->n_channels;
	//printf("format header size is %hu\n",w_fmt->header_size);
	//printf("bytes per channel is %hu\n",bytes_p_channel);
	
	/*look for "data" since there is random metadata between "fmt " and data*/
	long findex=ftell(f);
	char data_found = 0;
	while(data_found==0)
	{
		fseek(f,findex,SEEK_SET);
		if (fread(d_hdr,sizeof(data_header),1,f)==1)
		{
			if (strncmp(d_hdr->header,"data",4)!=0)
			{
				findex += 1;
			}
			else
			{
				data_found = 1;
			}
		}
		else
		{
			printf("no data section found.\n");
			return -1;
		}
	}
	
	if(strncmp(d_hdr->header,"data",4)!=0)
	{
		printf("no data section following, instead: %s\n",d_hdr->header);
		return -1;
	}
	
	/*allocate an array of sample unions which holds all samples, they are stored interleaved*/
	int n_frames = d_hdr->data_size/(unsigned int)w_fmt->frame_size;
	int cnt = 0;
	*sample_array = (sample*)malloc(n_frames*w_fmt->frame_size);
	//printf("reading %d frames\n\n",n_frames);
	*array_size=n_frames;
	for(cnt=0;cnt<n_frames;cnt++)
	{
		// read one sample, convert it to 16 bit stereo if needed
		unsigned char * bfr;
		bfr=(unsigned char*)malloc(bytes_p_channel);
		if (w_fmt->n_channels==1)
		{
			fread(bfr,bytes_p_channel,1,f);
			if (bytes_p_channel == 1)
			{
				s_l.c_val[0] =0;
				s_l.c_val[1] = *bfr;
			}
			else 
			{
				s_l.c_val[0] =*(bfr+bytes_p_channel-2);
				s_l.c_val[1] =*(bfr+bytes_p_channel-1);
			}
			s_r.i_val = s_l.i_val;
		}
		else
		{
			fread(bfr,bytes_p_channel,1,f);
			if (bytes_p_channel == 1)
			{
				s_l.c_val[0] =0;
				s_l.c_val[1] = *bfr;
			}
			else 
			{
				s_l.c_val[0] =*(bfr+bytes_p_channel-2);
				s_l.c_val[1] =*(bfr+bytes_p_channel-1);
			}
			fread(bfr,bytes_p_channel,1,f);
			if (bytes_p_channel == 1)
			{
				s_r.c_val[0] =0;
				s_r.c_val[1] = *bfr;
			}
			else 
			{
				s_r.c_val[0] =*(bfr+bytes_p_channel-2);
				s_r.c_val[1] =*(bfr+bytes_p_channel-1);
			}
		}
		*(*sample_array+cnt) = s_l;
		*(*sample_array+cnt+2) = s_r;
	}

	fclose(f);
	return 0;
}
