#include <alsa/asoundlib.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>

#include "wave_file_reader.h"
#include "gpio.h"

#define ZERO_LEVEL 32768
#define Q_NOTE_DELAY 250000
#define PIN_NR 17
#define BIT_0_PIN 27
#define BIT_1_PIN 22
#define BIT_2_PIN 23


unsigned short get_phase_increment(short);
int init_sound_device(char *,snd_pcm_t**,unsigned char **,snd_pcm_uframes_t*);
void* console_listener(void*);
void* beatmaker(void*);
void* gpio_listener(void*);
void* selector_listener(void*);

int cnt1,cnt2,cnt3,cnt4,cnt5,cnt6;
int* current_cnt;
int * cnt_array;
char is_running = 1;
int fd;

void  finish(int sig)
  {
  is_running = 0;
  }

int main(int argc, char** argv)
{
	snd_pcm_t* pcm_handle;
	unsigned char* data;
	snd_pcm_uframes_t frames;
	char* dname = strdup("plughw:0,0");
	int ret;
	int cnt_frames;
	int q=0;

	sample sample_played_l;
	sample sample_played_r;
	sample** sample_matrix;
	sample* wave_array_1;
	sample* wave_array_2;
	sample* wave_array_3;
	sample* wave_array_4;
	sample* wave_array_5;
	sample* wave_array_6;
	short sum_l, sum_r;
	char * fname;
	
	int wave_size_1,wave_size_2,wave_size_3,wave_size_4,wave_size_5,wave_size_6;
	int * wave_sizes;
	pthread_t keyboard_thread;
	pthread_t beatmaker_thread;
	pthread_t gpio_thread;
	pthread_t sel_thread;

    struct sigaction sig_struct;
    sig_struct.sa_handler = finish;
    sigemptyset(&sig_struct.sa_mask);
    sig_struct.sa_flags = 0;
    sigaction(SIGINT,&sig_struct,NULL);
	
	if (argc != 1)
	{
		printf("Usage without any arguments\n");
		return(1);
	}
	
	ret = init_sound_device(dname,&pcm_handle,&data,&frames);
	if (ret==0)
	{
		printf("sound device initialization successful, audio buffer holds %d frames\n",frames);
	}
	
    cnt_array = (int*)malloc(6*sizeof(int));
	sample_matrix=(sample**)malloc(6*sizeof(sample*));
	wave_sizes=(int*)malloc(6*sizeof(int));

	for (q=0;q<6;q++)
	{
		sprintf(fname,"track00%d.wav",q+1);
		printf(fname);
		printf("\n");
		read_wav_to_array(fname,sample_matrix+q,wave_sizes+q);
		*(cnt_array+q)=-1;
	}


	current_cnt = cnt_array+0;

	gpio_init(PIN_NR,MODE_READ);
	gpio_edge_prepare(PIN_NR,E_BOTH);
	

    char bfr[BUFFER_SIZE];

    memset(bfr,0,BUFFER_SIZE);
    sprintf(bfr,"/sys/class/gpio/gpio%d/value",PIN_NR);
    fd = open(bfr,O_RDONLY | O_NONBLOCK);

	gpio_init(BIT_0_PIN,MODE_READ);
    gpio_init(BIT_1_PIN,MODE_READ);
    gpio_init(BIT_2_PIN,MODE_READ);
	printf("GPIO initialized\n");

	//pthread_create(&keyboard_thread,NULL,console_listener,NULL);
	//pthread_create(&beatmaker_thread,NULL,beatmaker,NULL);
	pthread_create(&gpio_thread,NULL,gpio_listener,NULL);
    pthread_create(&sel_thread,NULL,selector_listener,NULL);
	
	while(is_running==1)
	{

		/* do thing which alter every periodsize samples an which are calculation intensive*/
		
		for (cnt_frames=0;cnt_frames<frames;cnt_frames++)
		{
			// fill in a period of data
			sum_l=0;
			sum_r=0;
			
			for(q=0;q<6;q++)
			{
				if (*(cnt_array+q)>-1)
				{
					sample_played_l = *(*(sample_matrix+q) + *(cnt_array+q));
					*(cnt_array+q) = *(cnt_array+q)+1;
					sample_played_r = *(*(sample_matrix+q) + *(cnt_array+q));
					*(cnt_array+q) = *(cnt_array+q)+1;
					if (*(cnt_array+q) >= *(wave_sizes+q))
					{
						*(cnt_array+q)=-1;
					}
					sum_l += ((short)sample_played_l.i_val)/6;
					sum_r += ((short)sample_played_r.i_val)/6;
				}
			}

			
			data[4*cnt_frames+0] = (unsigned char)sum_l;
			data[4*cnt_frames+1] = sum_l >> 8;
			data[4*cnt_frames+2] = (unsigned char)sum_r;
			data[4*cnt_frames+3] = sum_r >> 8;
			
		}
		//printf("a period written\n");
		snd_pcm_sframes_t pcm_write_ret;
			
		while ((pcm_write_ret = snd_pcm_writei(pcm_handle, data, frames)) < 0) {
			snd_pcm_prepare(pcm_handle);
			if (pcm_write_ret == -EPIPE)
			{
			    fprintf(stderr, "Write Error, Underrun\n");
			}
			else if(pcm_write_ret == -EBADFD)
			{
				fprintf(stderr, "Write Error, Wrong PCM state\n");
			}
			else if(pcm_write_ret == -ESTRPIPE)
			{
				fprintf(stderr, "Write Error, suspended stream\n");
			}
			else
			{
				fprintf(stderr,"Write Error, unknown %s\n",pcm_write_ret);
			}
		}
	}

	gpio_close(PIN_NR);
	snd_pcm_drain(pcm_handle);
	
}


int init_sound_device(char * pcm_name,snd_pcm_t** pcm_handle, unsigned char ** buffer,snd_pcm_uframes_t* frames)
{
	int rate = 44100; /* Sample rate */
	int periods = 32;       /* Number of periods */
    snd_pcm_uframes_t periodsize = 256; /* Periodsize (bytes) */
	
	snd_pcm_stream_t stream = SND_PCM_STREAM_PLAYBACK;
	snd_pcm_hw_params_t *hwparams;

	snd_pcm_hw_params_alloca(&hwparams);

	if (snd_pcm_open(pcm_handle, pcm_name, stream, 0) < 0) {
		  fprintf(stderr, "Error opening PCM device %s\n", pcm_name);
		  return(-1);
		}
	
	if (snd_pcm_hw_params_any(*pcm_handle, hwparams) < 0) {
      fprintf(stderr, "Can not configure this PCM device.\n");
      return(-1);
    }
	
	if (snd_pcm_hw_params_set_access(*pcm_handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED) < 0) {
      fprintf(stderr, "Error setting access.\n");
      return(-1);
    }
	
	if (snd_pcm_hw_params_set_format(*pcm_handle, hwparams, SND_PCM_FORMAT_S16_LE) < 0) {
      fprintf(stderr, "Error setting format.\n");
      return(-1);
    }


    unsigned int exact_rate; 
	exact_rate = rate;
    if (snd_pcm_hw_params_set_rate_near(*pcm_handle, hwparams, &exact_rate, 0) < 0) {
      fprintf(stderr, "Error setting rate.\n");
      return(-1);
    }
    if (rate != exact_rate) {
      fprintf(stderr, "The rate %d Hz is not supported by your hardware. ==> Using %d Hz instead.\n", rate, exact_rate);
    }
	
	    /* Set number of periods. Periods used to be called fragments. */ 
    if (snd_pcm_hw_params_set_periods(*pcm_handle, hwparams, periods, 0) < 0) {
      fprintf(stderr, "Error setting periods.\n");
      return(-1);
    }
    snd_pcm_uframes_t exact_buffer_size;
	exact_buffer_size = (periodsize * periods)>>2;
    if (snd_pcm_hw_params_set_buffer_size_near(*pcm_handle, hwparams, &exact_buffer_size) < 0) {
      fprintf(stderr, "Error setting buffersize.\n");
      return(-1);
    }
	
	if (exact_buffer_size != (periodsize * periods)>>2)
	{
		fprintf(stderr,"Hardware not happy with buffersize %s, setting %s instead",(periodsize * periods)>>2, exact_buffer_size);
	}
	
	  if (snd_pcm_hw_params(*pcm_handle, hwparams) < 0) {
		fprintf(stderr,
				"unable to set hw parameters\n");
		return(-1);
	  }
	
	*frames = periodsize >> 2;
	*buffer = (unsigned char *)malloc(periodsize);
	return(0);
}


void* console_listener(void* args)
{
	char answer[8];
	while(is_running==1)
	{
		printf("enter 1 to 6 to play a sound or q to quit:\n");
		scanf("%s",answer);
		if(strcmp(answer,"1")==0)
		{
			*(cnt_array+0)=0;
		}
		else if(strcmp(answer,"2")==0)
		{
			*(cnt_array+1)=0;
		}
		else if(strcmp(answer,"3")==0)
		{
			*(cnt_array+2)=0;
		}
		else if(strcmp(answer,"4")==0)
		{
			*(cnt_array+3)=0;
		}
		else if(strcmp(answer,"5")==0)
		{
			*(cnt_array+4)=0;
		}
		else if(strcmp(answer,"6")==0)
		{
			*(cnt_array+5)=0;
		}
		else if(strcmp(answer,"q")==0)
		{
			is_running=0;
		}
	}
	return NULL;
}

void* beatmaker(void* args)
{
	while(is_running==1)
	{
		//1
		*(cnt_array+2)=0;
		*(cnt_array+0)=0;
		usleep(Q_NOTE_DELAY);
		
		//2
		*(cnt_array+2)=0;
		usleep(Q_NOTE_DELAY);
		
		//3
		*(cnt_array+2)=0;
		*(cnt_array+3)=0;
		usleep(Q_NOTE_DELAY);

		//4
		*(cnt_array+2)=0;
		usleep(Q_NOTE_DELAY);
		
		//5
		*(cnt_array+2)=0;
		*(cnt_array+4)=0;
		usleep(Q_NOTE_DELAY);
		
		//6
		*(cnt_array+2)=0;
		usleep(Q_NOTE_DELAY);
		
		//7
		*(cnt_array+2)=0;
		*(cnt_array+0)=0;
		usleep(Q_NOTE_DELAY);

		//8
		*(cnt_array+2)=0;
		*(cnt_array+5)=0;
		usleep(Q_NOTE_DELAY);
	}
	return NULL;
}

void * gpio_listener(void * args)
{
	int value;
	while(is_running == 1)
	{
		value = wait_for_edge_of(fd);
		if (value == 0)
		{
			*current_cnt=0;
		}
		usleep(30000);
	}
	return NULL;
}

void * selector_listener(void * args)
{
/*
bit 0: gpio 27, bit 1: gpio 22, bit 2: gpio 23
*/
	int bit_0,bit_1,bit_2;
    int val; 
	while (is_running==1)
	{
		usleep(100000);
		bit_0=gpio_read(BIT_0_PIN);
		bit_1=gpio_read(BIT_1_PIN);
		bit_2=gpio_read(BIT_2_PIN);
		val=(~(bit_0 | bit_1 << 1 | bit_2 << 2))&0x07;
		current_cnt=cnt_array+val;
		//printf("current selector value is %d\n",val);
	}
	return NULL;
}


unsigned short get_phase_increment(short note_number)
{   /* 432 Hz is note number 0, array goes from -41 to 24 */
	/*increments computed using excel sheet */
	unsigned short pi;
	short note_index;
	//printf("in phase_increment,note number is %d\n",note_number);
	unsigned short increments[]={120,127,135,143,151,160,170,180,191,202,214,227,240,255,270,286,303,321,340,360,382,404,428,454,481,510,540,572,606,642,680,721,763,809,857,908,962,1019,1080,1144,1212,1284,1360,1441,1527,1618,1714,1816,1924,2038,2159,2288,2424,2568,2721,2882,3054,3235,3428,3632,3848,4076,4319,4576,4848,5136};
	//printf("before index computations\n");
	note_index = (short)(note_number+41);
	//printf("the index is %d",note_index);
	return increments[note_index];
}
