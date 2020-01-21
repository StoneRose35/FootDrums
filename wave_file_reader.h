

typedef struct 
{
	char riff[4];
	unsigned int filesize;
	char wave[4];
} wav_header;

typedef struct 
{
	char id[4];
	unsigned int header_size;
	unsigned short format_tag;
	unsigned short n_channels;
	unsigned int sample_rate;
	unsigned int data_rate;
	unsigned short frame_size;
	
} wav_format;

union sample
{
	char c_val[2];
	unsigned short i_val;
};

typedef struct
{
	char header[4];
	unsigned int data_size;
} data_header;

int read_wav_to_array(char *, sample **,int *);
