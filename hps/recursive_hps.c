#include <unistd.h>
#include <malloc.h>
#include <math.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <time.h>

#include 	"hps.h"
#include 	"exception.h"
//#include 	"codeConvert.h"
#include 	"searchtrie.h"
#include 	"hps_keyword.h"

#define	HANGL_MASK	0x80

//#include 	"detectCandidate.h"
/* ansi terminal coloring */

#define	MAX_TRY_AGAIN	1024
#ifdef	EXPLORE
#define RESET       "</font>"
#define BLACK       "<font color=black>"
#define RED         "<font color=red>"
#define GREEN       "<font color=green>"
#define YELLOW      "<font color=yellow>"
#define BLUE        "<font color=blue>"
#define MAGENTA     "<font color=magenta>"
#define CYAN        "<font color=cyan>"
#define WHITE       "<font color=white>"

#define	ENDLINE	"<br>\n"
#else
#define CLEAR       "\e[0m"
#define RESET       "\e[0m"
#define BOLD        "\e[1m"
#define DARK        "\e[2m"
#define UNDERLINE   "\e[4m"
#define UNDERSCORE  "\e[4m"
#define BLINK       "\e[5m"
#define REVERSE     "\e[7m"
#define CONCEALED   "\e[8m"
#define BLACK       "\e[30m"
#define RED         "\e[31m"
#define GREEN       "\e[32m"
#define YELLOW      "\e[33m"
#define BLUE        "\e[34m"
#define MAGENTA     "\e[35m"
#define CYAN        "\e[36m"
#define WHITE       "\e[37m"
#define ON_BLACK    "\e[40m"
#define ON_RED      "\e[41m"
#define ON_GREEN    "\e[42m"
#define ON_YELLOW   "\e[43m"
#define ON_BLUE     "\e[44m"
#define ON_MAGENTA  "\e[45m"
#define ON_CYAN     "\e[46m"
#define ON_WHITE    "\e[47m"

#define	ENDLINE	"\n"
#endif

#define	PUSH	"PUSH"
#define	POP		"POP"

#define	HAN_MASK(c)( ((c & 0x80) == 0x80) ? 1 : 0)
int	backtracking( HPS * hps, conditions_t *conditions, char *in,  int index , space_ret_t spaceret , check_mode_t check_mode );
extern uint_t   get_all_node( const search_trie_t	*trie, uint_t address,uchar_t *key,  uint_t *pointer,uint_t *ynext, uint_t *result);

static int	grammar[26][26]={{0,},};
hps_space_t hps_spacing;
#define		HANGUL_SIZE	2
#define		GRAMMAR(c1, c2) ( grammar[c1 - 'a'][c2 - 'a'] = 1  )
/* limit ???? ??ŭ ?߰? */
char    *hps_strlcat( char   *dst, const char    *src, SIZE_T    *limit )
{
	char    *p;

	if( !src )
		return dst;
	if( *src == '\0' ) 
		return dst;
	if( !dst )
		return NULL;

	p = dst;
	while( *p++ );
	p--;

	while( *src != '\0' && *limit > 0 )
	{
		*p++ = *src++;
		( *limit )--;
		if( *src != '\0' && *src & 0x80 )
		{
			if( *limit > 0 )
			{
				*p++ = *src++;
				( *limit )--;
			}
			else
			{
				p--;
				break;
			}
		}

	}
	*p = '\0';
	return p;

}



void	print_rate( int curr_blank_rate, char *msg )
{
	fprintf(stdout,"\t\t\t\t\t\t%scurr rate = %s%02d%%%s%s",msg,ON_RED, curr_blank_rate,RESET,ENDLINE);
}

char	*print_stat( space_ret_t space_ret )
{
	switch( space_ret )
	{
		case INSERT_SPACE:				return "INSERT_SPACE";
		case NO_NEED_SPACE:				return "NO_NEED_SPACE";
		case CAN_NOT_MAKE_A_DECISION:	return "CAN_NOT_MAKE_A_DECISION";
	}
	return "NO_PRINT_OPTION";
}

void	print_left_right_condition( space_ret_t left, space_ret_t right)
{
	hps_debug(printf("left %s%s%s, right %s%s%s%s",ON_BLUE, hps_debug(print_stat( left )),RESET,ON_RED, hps_debug(print_stat( right )),RESET, ENDLINE));
}

void	print_string( char *in, int index )
{
	if( HAN_MASK(in[index+0]))
	{
		fprintf(stdout,"%s%s%c",ENDLINE,GREEN,in[index+0]);
		fprintf(stdout,"%c",in[index+1]);
		if( HAN_MASK(in[index+2]) )
		{
			fprintf(stdout,"%c",in[index+2]);
			fprintf(stdout,"%c%s%s",in[index+3],RESET,ENDLINE);
		}
		else
		{
			fprintf(stdout,"%c%s%s",in[index+1],RESET,ENDLINE);
		}

	}
	else
	{
		fprintf(stdout,"%s%s%c",ENDLINE,GREEN,in[index+0]);
		if( HAN_MASK(in[index+1]) )
		{
			fprintf(stdout,"%c",in[index+1]);
			fprintf(stdout,"%c%s%s",in[index+2],RESET,ENDLINE);
		}
		else
		{
			fprintf(stdout,"%c%s%s",in[index+1],RESET,ENDLINE);
		}

	}
}

char	*_replace	( HPS	*hps,int index, char	*out,char	* presult,SIZE_T	*len)
{
	char result[1024];
	char *s = result;
	hps_search_trie_replace( hps->replace, presult, s );
	if( index ==0 && *s == ' ' ) s++;
	
	//*out++ = ' ';// ��???? ?????ϴ? ???? ��?? ?տ? ????�� ???? ???ƴ޶??? ??û( ??????, 2011/01/03 )
	while( *s != '\0' )
	{
		*out = *s;
		out++; s++;
	}
	*out = '\0';

		
	return out+strlen(out);
}



char	*_digit		( HPS	*hps,int index,  char	*out,char	* presult,SIZE_T	*len)
{
	char	preChar = '\0';
	if(index > 0)	preChar = hps->position->tag[index-1];
	else			preChar = -1;

	if( preChar != '\0' && ( preChar == HPS_TAG_VERB || preChar == 'j' || preChar == 'c' ))
	{
		out = hps_strlcat( out,HPS_BLANK,len );
	}
	out = hps_strlcat( out,presult,len );
	return out;
}

char	*_english		( HPS	*hps,int index,  char	*out,char	* presult,SIZE_T	*len)
{
	char	preChar = '\0';
	if(index > 0)	preChar = hps->position->tag[index-1];
	else			preChar = -1;

	if( preChar != '\0' && ( preChar == HPS_TAG_VERB || preChar == 'j'|| preChar == 'c' || preChar == 'e' ))
	{
		out = hps_strlcat( out,HPS_BLANK,len );
	}
	out = hps_strlcat( out,presult,len );
	return out;
}

char	*_preback	( HPS	*hps,int index,  char	*out,char	* presult,SIZE_T	*len)
{
	if( index > 0 )
	{
		out = hps_strlcat( out,HPS_BLANK,len );
	}
	out = hps_strlcat( out,presult,len   );
	out = hps_strlcat( out,HPS_BLANK,len );
	return out;
}

char	*_back		( HPS	*hps,int index,  char	*out,char	* presult,SIZE_T	*len)
{
	char	preChar = '\0';
	if(index > 0)	preChar = hps->position->tag[index-1];
	else			preChar = 0;

	if( preChar == 'c' || preChar == 'v' || preChar == 'j' || preChar == 'r'  )
	out = hps_strlcat( out,HPS_BLANK,len );

	out = hps_strlcat( out,presult,len   );
	out = hps_strlcat( out,HPS_BLANK,len );
	return out;
}

char	*_pre		( HPS	*hps,int index,  char	*out,char	* presult,SIZE_T	*len)
{

	if( index > 0 )
	{
		if( (isdigit(*(out-1)) && isdigit(*presult)));
		else if( (isalpha(*(out-1)) && isalpha(*presult)));
		else  	out = hps_strlcat( out,HPS_BLANK,len );
	}
	out = hps_strlcat( out,presult,len   );
	return out;
}

char	*_nq		( HPS	*hps,int index,  char	*out,char	* presult,SIZE_T	*len)
{
	char	preChar = '\0';
	if(index > 0)	preChar = hps->position->tag[index-1];
	else			preChar = 0;

	if( hps->strong != 1 ){
		if( preChar == 'c' || preChar == 'v' || preChar == 'j' || preChar == 'e' || preChar == 'h' || preChar == 'r'|| preChar == 'p' )
		out = hps_strlcat( out,HPS_BLANK,len );
	}else{
		out = hps_strlcat( out,HPS_BLANK,len );
	}
	out = hps_strlcat( out,presult,len   );
	return out;
}

char	*_noun		( HPS	*hps,int index,  char	*out,char	* presult,SIZE_T	*len)
{

	char	preChar = '\0';
	if(index > 0)	preChar = hps->position->tag[index-1];
	else			preChar = 0;

	if( hps->strong != 1 ){
		if( preChar == 'c' || preChar == 'v' || preChar == 'j' || preChar == 'e' || preChar == 'r' || preChar == 'p' ){
			hps->statistics->mark[hps->position->row[index]] = INSERT_SPACE;
			out = hps_strlcat( out,HPS_BLANK,len );
		}
	}else{
		hps->statistics->mark[hps->position->row[index]] = INSERT_SPACE;
		out = hps_strlcat( out,HPS_BLANK,len );
	}
	out = hps_strlcat( out,presult,len );
	return out;
}

char	*_hada		( HPS	*hps,int index,  char	*out,char	* presult,SIZE_T	*len)
{
	char	preChar = '\0';
	if(index > 0)	preChar = hps->position->tag[index-1];
	else			preChar = 0;

	if( hps->strong != 1 ){
		if( preChar == 'c' || preChar == 'v' || preChar == 'j' || preChar == 'e' || preChar == 'r' || preChar == 'p' )
		out = hps_strlcat( out,HPS_BLANK,len );
	}else{
		out = hps_strlcat( out,HPS_BLANK,len );
	}
	out = hps_strlcat( out,presult,len );
	return out;
}

char	*_xsp		( HPS	*hps,int index,  char	*out,char	* presult,SIZE_T	*len)
{
	char	preChar = '\0';
	if(index > 0)	preChar = hps->position->tag[index-1];
	else			preChar = 0;

	if( preChar != 'h' )
	{
		out = hps_strlcat( out,HPS_BLANK,len );
	}
	out = hps_strlcat( out,presult,len );
	return out;
}

char	*_verb		( HPS	*hps,int index,  char	*out,char	* presult,SIZE_T	*len)
{

	if( index > 0 )
	{
		out = hps_strlcat( out,HPS_BLANK,len );
	}
	out = hps_strlcat( out,presult,len   );
	return out;
}

char	*_josa		( HPS	*hps,int index,  char	*out,char	* presult,SIZE_T	*len)
{
	char	preChar = '\0';
	if(index > 0)	preChar = hps->position->tag[index-1];
	else			preChar = 0;

	if( ( preChar != '\0' && ( preChar == HPS_TAG_EUMI || preChar == HPS_TAG_JOSA )  && *( out-1 ) == ' '  ) )
	{
		out--;
		*out = '\0';
	}

	out = hps_strlcat( out,presult,len   );
	return out;
}

char	*_eumi		( HPS	*hps,int index,  char	*out,char	* presult,SIZE_T	*len)
{
	char	preChar = '\0';
	if(index > 0)	preChar = hps->position->tag[index-1];
	else			preChar = 0;

	if( ( preChar != '\0' && ( preChar == HPS_TAG_EUMI || preChar == HPS_TAG_JOSA )  && (*(out-1) != '\0' && *( out-1 ) == ' '  )))
	{
		*( out-1 ) = '\0';
		out--;
	}

	out = hps_strlcat( out,presult,len   );
	//out = hps_strlcat( out,HPS_BLANK,len );
	return out;

}

char	*_space	( HPS	*hps,int index,  char	*out,char	* presult,SIZE_T	*len)
{
	if( index == 0 )
	{
		out = hps_strlcat( out, presult,len );
		out = hps_strlcat( out, HPS_BLANK,len );
	}
	else
	{
		out = hps_strlcat( out, &presult[0], len );
		out = hps_strlcat( out, HPS_BLANK,len );
	}
	return out;
}

char	*_nospace	( HPS	*hps,int index,  char	*out,char	* presult,SIZE_T	*len)
{
	out = hps_strlcat( out, &presult[0], len );
	return out;
}

char	*_default	( HPS	*hps,int index,  char	*out,char	* presult,SIZE_T	*len)
{
	char	preChar = '\0';
	if(index > 0)	preChar = hps->position->tag[index-1];
	else			preChar = 0;

	if( preChar == 'j' || preChar == 'e')
	{
		out = hps_strlcat( out,HPS_BLANK,len );
	}
	out = hps_strlcat( out,presult,len );

	return out;
}

hps_sub_function	function_call[26] = 
{
	_digit,		//'a'//
	_back,		//'b'//
	_xsp,		//'c'//
	_digit,		//'d'//
	_eumi,		//'e'//
	_english,	//'f'//
	_default,	//'g'//
	_hada,		//'h'//
	_default,	//'i'//
	_josa,		//'j'//
	_noun,		//'k'//
	_default,	//'l'//
	_default,	//'m'//
	_noun,		//'n'//
	_default,	//'o'//
	_pre,		//'p'//
	_nq,		//'q'//
	_replace,	//'r'//
	_default,	//'s'//
	_preback,	//'t'//
	_default,	//'u'//
	_verb,		//'v'//
	_default,	//'w'//
	_nospace,	//'x'//
	_space,		//'y'//
	_nospace,	//'z'//
};


void	ArrangeBlank( char	*out )
{
	char   *r;
	char   *w;
	int    blank = 0;

	w = out;
	while( *out != '\0' ){ if( *out == ' ' ) out++;  else break; }

	r = out;

	while( *r != '\0' )
	{

		if( *r != ' ' )
		{

			*( w++ ) = *( r++ );
			blank = 1;

		}else{

			if( blank )
			{

				*( w++ ) = *( r++ );
				blank = 0;

			}else{

				r++;
				blank = 1;
			}
		}
	}

	w--;
	while( w != out ){
	 if( *w == ' ' ) w--;
	 else break; 
	}
	w++;
	*w = '\0';
}


char	cut_and_backup( HPS *hps, char **in, int index )
{
	char	backup;
	backup = (*in)[hps->position->row[index+1]];
	(*in)[hps->position->row[index+1]] = '\0';
	return backup;
}

void	recover( HPS *hps, char **in, int index, char backup )
{
	(*in)[hps->position->row[index+1]] = backup;
}


int	hps_statistics_push ( HPS *hps, int row, int *freq_array )
{
	int i;
	if( row >= MAX_STACK )		return FAIL;
	if( freq_array == NULL )
	{
		memset( hps->statistics->freq_array[row].frequency, 0, sizeof(double)*MAX_STAT);
		return FAIL;
	}
	for( i = 0; i < MAX_STAT; i++ )
	{
		hps->statistics->freq_array[row].frequency[i] = freq_array[i];
	}
	hps->statistics->index = row;
	return SUCC;
}

void	put_statistics_mark( HPS *hps , int index, space_ret_t space )
{
#ifdef	DEBUG
	char * msg[] = {"INSERT_SPACE","NO_NEED_SPACE","CAN_NOT_MAKE_A_DESICION"};
#endif
	hps_debug(fprintf(stdout,"[%s:%d] statistics mark = %d(%s)\n",__FUNCTION__,__LINE__,index,msg[space-1]));
	hps->statistics->mark[index] = space;
}

char	get_statistics_tag( HPS *hps, int idx, space_ret_t space )
{
	if(idx >= 0)	
	{
		switch ( space )
		{
			case INSERT_SPACE:
				put_statistics_mark( hps, idx, INSERT_SPACE );
				hps->statistics->mark[idx] = INSERT_SPACE;
				return SPACE_O;
			case NO_NEED_SPACE:
				put_statistics_mark( hps, idx, NO_NEED_SPACE );
				return SPACE_X;
			case CAN_NOT_MAKE_A_DECISION :
				put_statistics_mark( hps, idx, CAN_NOT_MAKE_A_DECISION );
				return SPACE_Z;
		}
	}
	hps->statistics->mark[idx] = NO_NEED_SPACE;
	return SPACE_X;
}

void    print_hps_check( HPS *hps, char *in ,int col, int down, char currenttag,char *msg)
{
	char    backup;

	backup    = in[col+1];
	in[col+1] = '\0';

	if(strcmp( msg,PUSH ) == 0)
		printf("%s%s%s row = %02d\tcol = %02d\ttag = %c\tindex=%d\t\tstring = %s%s",ON_BLUE,msg, RESET, down, col, (char)currenttag, hps->position->index,&in[down], ENDLINE);
	else
		printf("%s%s%s row = %02d\tcol = %02d\ttag = %c\tindex=%d\t\tstring = %s%s",ON_RED,msg, RESET, down, col, (char)currenttag, hps->position->index,&in[down], ENDLINE);
	in[col+1] = backup;
}

void trim( HPS *hps, char * out )
{
	char *r;
	while( *out != '\0' )
	{
			if( *out == ' ' ) out++;
			else              break; 
	}

	r = out;
	
	while( *r != '\0' )
	{
		if( *r == ' ' ) hps->blank++;
		r++;
	}


	r--;
	while( r != out )
	{
		if( *r == ' ' )
		{
			hps->blank--;
			r--;
		}
		else            break; 
	}
	r++;
	*r = '\0';
}


int	hps_make_word( HPS	*hps, char *in, SIZE_T size,  char *out )
{
	int		index;
	char	backup = 0;
	char	*oout;
	SIZE_T 	len = size-1;
	*out  = '\0';
	oout = out;


	for(index = 0; index < hps->position->index; index++)
	{
		if( (index + 1) != hps->position->index)
			backup = cut_and_backup( hps, &in, index );

		hps_debug(printf("%s[%c]%s -- %s%s%s%s",ON_RED,(char)hps->position->tag[index],RESET,GREEN, &in[hps->position->row[index]],RESET,ENDLINE));

		oout = function_call[( UCHAR )hps->position->tag[index] - START_INDEX](hps,index,oout,&in[hps->position->row[index]], &len);

		if( (index + 1) != hps->position->index)
			recover( hps, &in, index, backup );
		hps_debug(printf("OUT=%s%s%s\n",GREEN,out,RESET));
		//sprintf(temp,"/%c ",( UCHAR )hps->position.tag[index] );
		//oout = hps_strlcat( oout,temp ,&len);

	}

	if( hps->position->index == 0 )
		strcpy(oout, in);
	else	*oout = '\0';

	trim( hps, out );
	return hps->blank;
}

void	hps_clear_statistics_info( HPS * hps, SIZE_T size )
{
	int	i;
	int	j;
	for( i = 0; i < hps->position->index; i ++ )
	{
		hps->position->row[i] = 0;
		hps->position->col[i] = 0;
		hps->position->tag[i] = 0;
		hps->statistics->mark[i] = 0;
	}

	for( j = i; j <= size; j++ )
		hps->statistics->mark[j] = 0;

	hps->position->index        = 0;
}

int hps_load_dic ( HPS *hps, char   *in, SIZE_T size , table_t  *table )
{
	int     index;
	int	hps_search_trie();

	for(index = 0; index < size; index++)
	{
		table->len[index] = 0;
		table->len[index] = hps_search_trie( hps->trie, &in[index], &table->TABLE[index][index] );
		if( (in[index] & HANGL_MASK) == HANGL_MASK )
		{
			index++;
			table->len[index] = 0;
		}
	}

	return SUCC;
}


int	hps_init_grammar()
{
	GRAMMAR('o','a');
	GRAMMAR('o','c');
	GRAMMAR('o','n');
	GRAMMAR('o','h');
	GRAMMAR('o','q');
	GRAMMAR('o','v');
	GRAMMAR('o','f');
	GRAMMAR('o','b');
	GRAMMAR('o','b');
	GRAMMAR('o','d');
	GRAMMAR('o','p');
	GRAMMAR('o','z');
	GRAMMAR('o','t');
	GRAMMAR('o','x');
	GRAMMAR('o','y');
	GRAMMAR('o','z');
	GRAMMAR('o','r');

	GRAMMAR('a','t');
	GRAMMAR('a','q');
	GRAMMAR('a','n');
	GRAMMAR('a','n');
	GRAMMAR('a','h');
	GRAMMAR('a','v');
	GRAMMAR('a','t');
	GRAMMAR('a','b');
	GRAMMAR('a','p');
	GRAMMAR('a','d');
	GRAMMAR('a','x');
	GRAMMAR('a','y');
	GRAMMAR('a','z');
	GRAMMAR('a','r');
	GRAMMAR('a','k');

	GRAMMAR('f','t');
	GRAMMAR('f','q');
	GRAMMAR('f','n');
	GRAMMAR('f','h');
	GRAMMAR('f','v');
	GRAMMAR('f','t');
	GRAMMAR('f','b');
	GRAMMAR('f','p');
	GRAMMAR('f','d');
	GRAMMAR('f','d');
	GRAMMAR('f','x');
	GRAMMAR('f','y');
	GRAMMAR('f','z');
	GRAMMAR('f','r');
	GRAMMAR('f','k');
	GRAMMAR('f','j');

	GRAMMAR('d','t');
	GRAMMAR('d','q');
	GRAMMAR('d','n');
	GRAMMAR('d','h');
	GRAMMAR('d','v');
	GRAMMAR('d','t');
	GRAMMAR('d','b');
	GRAMMAR('d','p');
	GRAMMAR('d','d');
	GRAMMAR('d','f');
	GRAMMAR('d','x');
	GRAMMAR('d','y');
	GRAMMAR('d','z');
	GRAMMAR('d','r');
	GRAMMAR('d','k');
	GRAMMAR('d','j');

	GRAMMAR('n','t');
	GRAMMAR('n','n');
	GRAMMAR('n','q');
	GRAMMAR('n','j');
	GRAMMAR('n','h');
	GRAMMAR('n','v');
	GRAMMAR('n','x');
	GRAMMAR('n','y');
	GRAMMAR('n','z');
	GRAMMAR('n','d');
	//GRAMMAR('n','e');
	GRAMMAR('n','r');
	GRAMMAR('n','k');
	GRAMMAR('n','p');
	GRAMMAR('n','b');
	GRAMMAR('n','f');
		
	GRAMMAR('h','t');
	GRAMMAR('h','n');
	GRAMMAR('h','j');
	//GRAMMAR('h','e');
	GRAMMAR('h','d');
	GRAMMAR('h','h');
	GRAMMAR('h','q');
	GRAMMAR('h','v');
	GRAMMAR('h','b');
	GRAMMAR('h','x');
	GRAMMAR('h','y');
	GRAMMAR('h','z');
	GRAMMAR('h','r');
	GRAMMAR('h','k');
	GRAMMAR('h','p');

	GRAMMAR('j','f');
	GRAMMAR('j','d');
	GRAMMAR('j','t');
	//GRAMMAR('j','j');
	GRAMMAR('j','v');
	GRAMMAR('j','n');
	GRAMMAR('j','h');
	GRAMMAR('j','q');
	GRAMMAR('j','b');
	GRAMMAR('j','x');
	GRAMMAR('j','y');
	GRAMMAR('j','z');
	GRAMMAR('j','r');
	GRAMMAR('j','k');
	GRAMMAR('j','p');

	GRAMMAR('v','t');
	GRAMMAR('v','p');
	GRAMMAR('v','n');
	GRAMMAR('v','q');
	GRAMMAR('v','e');
	GRAMMAR('v','j');
	GRAMMAR('v','h');
	GRAMMAR('v','v');
	GRAMMAR('v','x');
	GRAMMAR('v','y');
	GRAMMAR('v','z');
	GRAMMAR('v','r');
	GRAMMAR('v','k');
	GRAMMAR('v','b');
	GRAMMAR('v','d');
	GRAMMAR('v','f');

	GRAMMAR('e','t');
	GRAMMAR('e','n');
	GRAMMAR('e','q');
	GRAMMAR('e','e');
	GRAMMAR('e','h');
	GRAMMAR('e','v');
	GRAMMAR('e','x');
	GRAMMAR('e','y');
	GRAMMAR('e','z');
	GRAMMAR('e','r');
	GRAMMAR('e','k');
	GRAMMAR('e','p');
	GRAMMAR('e','f');

	GRAMMAR('q','t');
	GRAMMAR('q','n');
	GRAMMAR('q','h');
	GRAMMAR('q','v');
	GRAMMAR('q','j');
	GRAMMAR('q','q');
	GRAMMAR('q','x');
	GRAMMAR('q','y');
	GRAMMAR('q','z');
	GRAMMAR('q','r');
	GRAMMAR('q','k');
	GRAMMAR('q','p');
	GRAMMAR('q','b');
	GRAMMAR('q','d');

	GRAMMAR('p','t');
	GRAMMAR('p','q');
	GRAMMAR('p','n');
	GRAMMAR('p','h');
	GRAMMAR('p','v');
	GRAMMAR('p','j');
	GRAMMAR('p','e');
	GRAMMAR('p','x');
	GRAMMAR('p','y');
	GRAMMAR('p','z');
	GRAMMAR('p','r');
	GRAMMAR('p','k');
	GRAMMAR('p','p');
	GRAMMAR('p','d');

	GRAMMAR('b','t');
	GRAMMAR('b','q');
	GRAMMAR('b','n');
	GRAMMAR('b','h');
	GRAMMAR('b','v');
	GRAMMAR('b','j');
	GRAMMAR('b','e');
	GRAMMAR('b','x');
	GRAMMAR('b','y');
	GRAMMAR('b','z');
	GRAMMAR('b','r');
	GRAMMAR('b','k');
	GRAMMAR('b','b');
	GRAMMAR('b','p');

	GRAMMAR('t','t');
	GRAMMAR('t','q');
	GRAMMAR('t','n');
	GRAMMAR('t','h');
	GRAMMAR('t','v');
	GRAMMAR('t','x');
	GRAMMAR('t','y');
	GRAMMAR('t','z');
	GRAMMAR('t','r');
	GRAMMAR('t','k');
	GRAMMAR('t','b');
	GRAMMAR('t','p');
	GRAMMAR('t','d');
	GRAMMAR('t','f');

	GRAMMAR('f','f');
	GRAMMAR('f','d');
	GRAMMAR('f','t');
	GRAMMAR('f','p');
	GRAMMAR('f','b');
	GRAMMAR('f','n');
	GRAMMAR('f','q');
	GRAMMAR('f','h');
	GRAMMAR('f','x');
	GRAMMAR('f','y');
	GRAMMAR('f','z');
	GRAMMAR('f','r');
	GRAMMAR('f','k');

	GRAMMAR('z','n');
	GRAMMAR('z','h');
	GRAMMAR('z','z');
	GRAMMAR('z','t');
	GRAMMAR('z','p');
	GRAMMAR('z','b');
	GRAMMAR('z','v');
	GRAMMAR('z','e');
	GRAMMAR('z','j');
	GRAMMAR('z','d');
	GRAMMAR('z','f');
	GRAMMAR('z','x');
	GRAMMAR('z','y');
	GRAMMAR('z','z');
	GRAMMAR('z','r');
	GRAMMAR('z','k');

	GRAMMAR('x','n');
	GRAMMAR('x','h');
	GRAMMAR('x','v');
	GRAMMAR('x','j');
	GRAMMAR('x','e');
	GRAMMAR('x','p');
	GRAMMAR('x','b');
	GRAMMAR('x','t');
	GRAMMAR('x','q');
	GRAMMAR('x','f');
	GRAMMAR('x','d');
	GRAMMAR('x','r');
	GRAMMAR('x','k');

	GRAMMAR('y','n');
	GRAMMAR('y','h');
	GRAMMAR('y','v');
	GRAMMAR('y','j');
	GRAMMAR('y','e');
	GRAMMAR('y','p');
	GRAMMAR('y','b');
	GRAMMAR('y','t');
	GRAMMAR('y','q');
	GRAMMAR('y','f');
	GRAMMAR('y','d');
	GRAMMAR('y','r');
	GRAMMAR('y','k');

	GRAMMAR('z','n');
	GRAMMAR('z','h');
	GRAMMAR('z','v');
	GRAMMAR('z','j');
	GRAMMAR('z','e');
	GRAMMAR('z','p');
	GRAMMAR('z','b');
	GRAMMAR('z','t');
	GRAMMAR('z','q');
	GRAMMAR('z','f');
	GRAMMAR('z','d');
	GRAMMAR('z','r');
	GRAMMAR('z','k');

	GRAMMAR('r','n');
	GRAMMAR('r','h');
	GRAMMAR('r','v');
	GRAMMAR('r','j');
	GRAMMAR('r','e');
	GRAMMAR('r','p');
	GRAMMAR('r','b');
	GRAMMAR('r','t');
	GRAMMAR('r','q');
	GRAMMAR('r','f');
	GRAMMAR('r','d');
	GRAMMAR('r','r');
	GRAMMAR('r','k');

	GRAMMAR('k','n');
	GRAMMAR('k','h');
	GRAMMAR('k','v');
	GRAMMAR('k','j');
	GRAMMAR('k','e');
	GRAMMAR('k','p');
	GRAMMAR('k','b');
	GRAMMAR('k','t');
	GRAMMAR('k','q');
	GRAMMAR('k','f');
	GRAMMAR('k','d');
	GRAMMAR('k','r');
	GRAMMAR('k','k');

//ncp?ϴٿ? ???? ?ϴ? ????�� ��?Ͽ? ???? ?±׿? ???? ?߰?
	GRAMMAR('c','t');
	GRAMMAR('c','p');
	GRAMMAR('c','n');
	GRAMMAR('c','q');
	GRAMMAR('c','e');
	GRAMMAR('c','j');
	GRAMMAR('c','h');
	GRAMMAR('c','v');
	GRAMMAR('c','x');
	GRAMMAR('c','y');
	GRAMMAR('c','z');
	GRAMMAR('c','r');
	GRAMMAR('c','k');
	GRAMMAR('c','b');
	GRAMMAR('c','d');
	GRAMMAR('c','f');

	GRAMMAR('n','c');
	GRAMMAR('h','c');
	GRAMMAR('j','c');
	GRAMMAR('e','c');
	GRAMMAR('q','c');
	GRAMMAR('p','c');
	GRAMMAR('b','c');
	GRAMMAR('t','c');
	GRAMMAR('z','c');
	GRAMMAR('x','c');
	GRAMMAR('y','c');
	GRAMMAR('z','c');
	GRAMMAR('r','c');
	GRAMMAR('k','c');
	GRAMMAR('v','c');
	GRAMMAR('d','c');
	return 0;
}

int	hps_grammar_is_full( HPS *hps )
{
	if( hps->position->index  >= MAX_STACK )
		return 1;
	return 0;
}


int	hps_grammar_is_empty( HPS *hps )
{
	if( hps->position->index == 0 )
		return 1;
	return 0;
}

int	hps_statistics_is_full( HPS *hps )
{
	if( hps->statistics->index  >= MAX_STACK )
		return 1;
	return 0;
}

int	hps_statistics_is_empty( HPS *hps)
{
	if( hps->statistics->index == 0 )
		return 1;
	return 0;
}

int	hps_grammar_push( HPS *hps, int row, int col, char tag )
{
	if( row == 0 && col == 0 )
	{
		if( tag != 'f' && tag != 'd' )
		return 0;
	}
	if( hps_grammar_is_full( hps ))	return -1;

	if( hps->position->index == 0 )
		hps->position->row[hps->position->index] = 0;
	else		
		hps->position->row[hps->position->index] = row;
	hps->position->col[hps->position->index] = col;
	hps->position->tag[hps->position->index] = tag;

	hps_debug(fprintf(stdout,"[%s:%d]row = %d, col = %d, tag = %c\n",__FUNCTION__,__LINE__,row,col,tag ));
	hps->position->index++;

	return 1;

}
int	hps_grammar_pop( HPS *hps )
{
	if( hps_grammar_is_empty(hps) )
		return 0;
	hps->position->index--;
	hps->position->row[hps->position->index] = 0;
	hps->position->col[hps->position->index] = 0;
	hps->position->tag[hps->position->index] = 0;
	return 1;
}

int	hps_grammar_get( HPS *hps, int *row, int *col )
{
	if( hps_grammar_is_empty(hps) )
		return 0;
	*row = hps->position->row[hps->position->index-1];
	*col = hps->position->col[hps->position->index-1];
	//*tag = hps->position->tag[hps->position->index-1];
	return 1;
}

int	search_trie_ngram(search_trie_t	*trie, char *keyword, char *result)
{

	int			i;
	int			size;
	int			length = 0;
	uint_t		ynext;
	uint_t 		data_address;
	uint_t 		address;
	uchar_t*	prt;
	uchar_t		key[1024];
	uint_t   	hps_get_node();

	if(trie->USE_TRIE != ST_USE_TRIE)
		return -1;

	*result = '\0';
	prt     = (uchar_t*)keyword;
	size    = strlen(keyword);

	address = trie->TRIE_FIRST_CHAR_ADDR[*prt];//???? ?ּ? ????

	if (address == 0)
		return -1;

	for(;;)
	{
		address = hps_get_node( trie, address, key, &prt, &data_address, &ynext );

		for( i = 0; key[i] != '\0'; i++ , prt++,result[length++]=0)
			if( *prt != key[i] )
				break;

		if( i == 0 ) 
		{
			if( ynext != 0 ) 	
				address = ynext;
			else	
				break;

			continue;
		}

		if(key[i] != '\0')			break;

		if( data_address != 0 )
		{
			strcpy(result, (char*)(trie->Ext_Trie + data_address));
		}

		if( *prt == '\0' || address == 0)		
			break;
	}

	return length;

}


int hps_load_ngram_statistics ( HPS * hps, char * in, int cur, SIZE_T readSize, ngram_t ngram)
{
	int  len;
	int  backup_i = -1;
	int  j     = 0;
	int  i     = 0;
	int  run   = 0;
	int  idx   = 0;
	int  limit = 0;
	int  *index= 0;
	int  bi_index[8]  = { 0, 1, 2, 3, 4, 5, 6,7 };
	int  tri_index[8] = { 0, 2, 4, 6,-1,-1,-1,-1};
	char *bp;
	char *token;
	char key[256];
	char val[256] = {0,};
	search_trie_t	*trie;
	unsigned int	fre   = 0;
	unsigned int	total = 0;
	unsigned int 	stat[MAX_STAT];
	val[0] = '\0';
	switch( ngram ){
			case BIGRAM :  index = bi_index;  limit = BI_LIMIT;  trie = hps->bigram;  break;	
			case TRIGRAM : index = tri_index; limit = TRI_LIMIT; trie = hps->trigram; break;
	}
						  
	for( i = cur; i < readSize+cur && run < 2; i++ , run++ )
	{
		//??��?˻?
		switch( ngram )
		{
			case BIGRAM  :
				if( !(HAN_MASK(in[i]) && HAN_MASK(in[i+2])) )
				{
					hps_statistics_push( hps, i, NULL );i++;
					hps_statistics_push( hps, i, NULL );
					continue;
				}
				break;
			case TRIGRAM :
				if( !(HAN_MASK(in[i]) && HAN_MASK(in[i+2]) && HAN_MASK(in[i+4])) )
				{
					hps_statistics_push( hps, i, NULL );i++;
					hps_statistics_push( hps, i, NULL );
					return -1;
				}
				break;
		}
		
		strncpy(key,&in[i],limit);
		key[limit] = '\0';
		len = search_trie_ngram( trie, key, val);
		memset( stat, 0, sizeof(int)*MAX_STAT);
		idx   = 0;
		total = 0;
		for( token = strtok_r(val," ",&bp); token != NULL; token = strtok_r(NULL," " ,&bp))
		{
			 fre = atoi(token); 	stat[index[idx++]] = fre;	total += fre; 
		}

		for( j = 0; j < MAX_STAT; j++ )	if( stat[j] > 0 && total > 0) stat[j] = (( stat[j] * 100 ) / total);
		
		if( idx == 0 ) 
		{
			if( backup_i >= 0 )
				hps_statistics_push( hps, backup_i, NULL );
			hps_statistics_push( hps, i, NULL );
			return -1;
		}
		else	
		{
			hps_statistics_push( hps, i, (int*)stat );
			backup_i = i;
		}

		if( HAN_MASK(in[i]) )    i++;
	}
	return 1;
}

int next_row( char *in, int row )
{
	if( in[row] & HANGL_MASK )
		return 2;
	else
		return 1;

}

void revise_condition( int *precond )
{
	switch( *precond )
	{
		case 0 :	*precond = 8; break;
		case 1 :	*precond = 9; break;
		case 2 :	*precond = 10; break;
		case 3 :	*precond = 11; break;
		default :				  break;
	}

}


int set_running_depth( int length )
{
	if( length > 2 )	return FOURTH;
	else				return ZERO;
}

/****************
   	0 : AAA
	2 : A AA
	4 : AA A
	6 : A A A
*****************/
int	trigram_direct_loop[][MAX_DEPTH] = {
	{ 0, 2, 4, 6,-1,-1,-1,-1},//0//???Ե?��=>????+??��/
	//{ 0, 4, 6,-1,-1,-1,-1,-1},//0//???ӵ???=>???ӵ?+??
	{-1,-1,-1,-1,-1,-1,-1,-1},//1
	{ 0, 1, 4, 6,-1,-1,-1,-1},//2
	{-1,-1,-1,-1,-1,-1,-1,-1},//3
	{ 2, 4, 6,-1,-1,-1,-1,-1},//4
	{-1,-1,-1,-1,-1,-1,-1,-1},//5
	{ 2, 4, 6,-1,-1,-1,-1,-1},//6
	{-1,-1,-1,-1,-1,-1,-1,-1},//7
	{-1,-1,-1,-1,-1,-1,-1,-1},//8
	{-1,-1,-1,-1,-1,-1,-1,-1},//9
	{-1,-1,-1,-1,-1,-1,-1,-1},//10
	{-1,-1,-1,-1,-1,-1,-1,-1},//11
	{ 0, 2, 4, 6,-1,-1,-1,-1} //12
};

/****************
   	0 : AA
   	1 :  AA
   	2 : A A
   	3 :  A A
   	4 : AA 
   	5 :  AA
   	6 : A A 
   	7 :  A A
*****************/
int	bigram_direct_loop[][MAX_DEPTH] = {
	//{ 0, 1, 2, 3, 4, 5, 6, 7},//0,1
	{ 0, 2, 4,-1,-1,-1,-1,-1},//0
	{ 0, 2, 4,-1,-1,-1,-1,-1},//1
	{ 1, 3, 5, 7,-1,-1,-1,-1},//2
	{ 1, 3, 5, 7,-1,-1,-1,-1},//3
	{ 2, 6,-1,-1,-1,-1,-1,-1},//4
	{ 0, 2, 4, 6,-1,-1,-1,-1},//5
	{ 1, 3, 5, 7,-1,-1,-1,-1},//6
	{ 1, 3, 5, 7,-1,-1,-1,-1},//7
	{ 4, 6,-1,-1,-1,-1,-1,-1},//8		0`
	{ 4, 6,-1,-1,-1,-1,-1,-1},//9		1`
	{ 5, 7,-1,-1,-1,-1,-1,-1},//10	2`
	{ 5, 7,-1,-1,-1,-1,-1,-1},//11	3`
	{ 0, 1, 2, 3, 4, 5, 6, 7} //12
};

/*int	trigram_direct_loop[][MAX_DEPTH] = {
	{ 0, 2, 4, 6},  //0
	{ 0, 2, 4, 6},  //1
	{ 0, 1, 3, 5},  //2
	{ 0, 2, 4, 6},  //3
	{ 0, 2, 4,-1},  //4
	{ 0, 2,-1,-1},  //5
	{ 0, 2,-1,-1},  //6
	{-1,-1,-1,-1},  //7
	{-1,-1,-1,-1},  //8		0`
	{-1,-1,-1,-1},  //9		1`
	{-1,-1,-1,-1},  //10	2`
	{-1,-1,-1,-1},  //11	3`
	{ 0, 2, 4, 6}   //12	8
};
int	bigram_direct_loop[][MAX_DEPTH] = {
	{ 0, 2, 4, 6},  //0		XXX
	{ 0, 2, 4, 6},  //1		OXX
	{ 1, 3, 5,-1},  //2		XOX
	{ 1, 3, 5,-1},  //3		OOX
	{ 0, 2, 4, 6},  //4		XXO
	{ 2, 6,-1,-1},  //5		OXO
	{ 3, 7,-1,-1},  //6		XOO
	{ 3, 7,-1,-1},  //7		OOO
	{ 4, 6,-1,-1},  //8		0`
	{ 4, 6,-1,-1},  //9		1`
	{ 5, 7,-1,-1},  //10	2`
	{ 5, 7,-1,-1},  //11	3`
	{ 1, 3, 5, 7}
};
 */


double* get_row_frequency( HPS *hps,int  row )
{
	return hps->statistics->freq_array[row].frequency;
}


void sync_candidate( summation_t *summation )
{
	int	i;
	if( summation->candidate != NULL && summation->cindex > 0 )
	{
		switch( summation->depth )
		{ 
			case 0 : break;
			case 1 :
				for( i = 0; i < summation->depth; i++ )
					summation->candidate[summation->cindex].idx[i] = summation->candidate[summation->cindex-1].idx[i];
				break;
		}

	}
}

void pop_status( summation_t *summation , int *row, int *precond )
{
	summation->depth--;
	*row     = summation->brow[summation->depth];
	*precond = summation->bprecond[summation->depth];
	summation->contstop = HPS_CONT;
}

void push_status( summation_t *summation, int *row, int *precond )
{
	summation->brow[summation->depth]     = *row;
	summation->bprecond[summation->depth] = *precond;
	if( summation->contstop == HPS_CONT )
	{
		summation->depth++;
		*precond = summation->curcond;
	}
}

void set_end_mark( summation_t *summation )
{
	if( summation->in[summation->row] & HANGL_MASK )
	{
		if( summation->in[summation->row+2] & HANGL_MASK )
		{
			if(summation->in[summation->row+4] == '\0' )
			{
				summation->end_mark = 1;
				return ;
			}
		}
	}
	summation->end_mark = 0;
}

void check_remain_size( summation_t *summation ,int row,  ngram_t ngram )
{
	int limit = 0;
	switch( ngram )
	{
		case BIGRAM :  limit = BI_LIMIT;  break;
		case TRIGRAM : limit = TRI_LIMIT; break;
	}
	if( summation->length - row < limit )
	{
		summation->contstop = HPS_STOP;
	}
	else
	{
		summation->contstop = HPS_CONT;
	}
}

#define T_00 0.7280363223
#define T_01 0.2719636777
#define T_10 0.9153650346
#define T_11 0.0391060913

double tr[2][2] = {{ T_00, T_01 },{ T_10, T_11 }};

void mux_tr( double *freq, int a, int b )
{
	int x, y;
	switch( a )
	{
		case 0 : x = 0; break;
		case 1 : x = 1; break;
		case 2 : x = 0; break;
		case 3 : x = 1; break;
		case 4 : x = 0; break;
		case 5 : x = 1; break;
		case 6 : x = 0; break;
		case 7 : x = 1; break;
		default : x = 0; break;
	}

	switch( b )
	{
		case 0 : y = 0; break;
		case 1 : y = 1; break;
		case 2 : y = 0; break;
		case 3 : y = 1; break;
		case 4 : y = 0; break;
		case 5 : y = 1; break;
		case 6 : y = 0; break;
		case 7 : y = 1; break;
		default : y = 0; break;
	}
	*freq *= tr[x][y];
}

int set_summation( summation_t *summation ,double *frequency,  int *precond , ngram_t ngram )
{
	switch( summation->contstop )
	{
		case HPS_CONT   : 
			break;
		case HPS_STOP : 
			if( summation->end_mark )
				revise_condition( precond );
			break;
	}

	switch( ngram )
	{
		case BIGRAM : 
			if((summation->curcond = bigram_direct_loop[*precond][summation->index[summation->depth]]) == -1)	return -1;
			break;
		case TRIGRAM :
			if((summation->curcond = trigram_direct_loop[*precond][summation->index[summation->depth]]) == -1)	return -1;
			break;
	}
	summation->candidate[summation->cindex].idx[summation->depth] = summation->curcond;
	if( summation->index[summation->depth] > 0 )
		sync_candidate( summation );

	if( frequency[summation->curcond] == 0 )	frequency[summation->curcond] = 1;

	switch( summation->contstop )
	{
		case HPS_CONT   : 
			switch( summation->depth ){
				case 0 : summation->tfreq[summation->depth][summation->index[summation->depth]] = frequency[summation->curcond]; break;
				case 1 : summation->candidate[summation->cindex].fre = summation->tfreq[summation->depth-1][summation->index[summation->depth-1]] * frequency[summation->curcond]; break;
			}
			mux_tr( &summation->candidate[summation->cindex].fre, *precond, summation->curcond );

			push_status( summation, &summation->row, precond );

			summation->remain = summation->length - summation->cur;
			set_end_mark( summation );
			summation->cur += next_row( summation->in, summation->row );
			summation->row += next_row( summation->in, summation->row );
			break;
		case HPS_STOP : 
			switch( summation->depth ){
				case 0 : summation->candidate[summation->cindex].fre = frequency[summation->curcond];	break;
				case 1 : summation->candidate[summation->cindex].fre = summation->tfreq[summation->depth-1][summation->index[summation->depth-1]] * frequency[summation->curcond]; break;
			}
			mux_tr( &summation->candidate[summation->cindex].fre, *precond, summation->curcond );

			push_status( summation, &summation->row, precond );

			summation->cindex++;
			break;
	}
	return 1;
}

void init_summation( summation_t *summation ,char *in,int  row,int  length, contstop_t  CONT,candidate_t *candidate )
{
	summation->length	= length;
	summation->in       = in;
	summation->depth	= 0;
	summation->cur		= 0;
	summation->row      = row;
	summation->contstop = HPS_CONT;
	summation->candidate= candidate;
	summation->remain   = length;
}

#define	BASE_PRECOND	12

int sum_frequency( HPS *hps, char *in, int row, int length,  candidate_t *candidate , int *precond , ngram_t ngram )
{
	double *frequency;
	int		initcond = *precond;
	summation_t *summation;
	summation = hps->summation;

	summation->cindex = 0;
	init_summation( summation , in, row, length, HPS_CONT, candidate );


	if((summation->max_depth = set_running_depth( length )) == ZERO ) return 0;
	hps_debug(printf("%s\n", &in[row]));

	check_remain_size( summation , row,  ngram );
	set_end_mark( summation );

	if( summation->contstop == HPS_STOP )	*precond = BASE_PRECOND;


	for( summation->index[summation->depth] = 0; summation->index[summation->depth] < MAX_DEPTH; summation->index[summation->depth]++ )//depth1
	{
		frequency = get_row_frequency( hps, summation->row );
		if( set_summation( summation, frequency, precond, ngram ) == -1 )	break;
		hps_debug(printf("[%d]freq = %f\n",summation->curcond, frequency[summation->curcond]));
		if( summation->contstop == HPS_STOP )
		{
			hps_debug(printf("+-----------------------------------------------TOTAL = %f\n", summation->candidate[summation->cindex-1].fre ));
			continue;
		}
		check_remain_size( summation , row,  ngram );

		for( summation->index[summation->depth] = 0; summation->index[summation->depth] < MAX_DEPTH; summation->index[summation->depth]++ )//depth4
		{
			summation->contstop = HPS_STOP;
			frequency = get_row_frequency( hps, summation->row );
			if( set_summation( summation, frequency, precond ,ngram) == -1 )	break;
			if( summation->contstop == HPS_STOP )
			{
				hps_debug(printf("+-----------------------[%d]freq = %f\n",summation->curcond, frequency[summation->curcond]));
				hps_debug(printf("+-----------------------------------------------TOTAL = %f\n",summation->candidate[summation->cindex-1].fre ));
			}
		}
		pop_status( summation, &summation->row, precond );
		pop_status( summation, &summation->row, precond );
		init_summation( summation , in, row, length, HPS_CONT, candidate );
		*precond = initcond;
	}

	return summation->cindex;
}





int max_frequency( candidate_t *candidate, int  cindex )
{
	int i;
	int	max_pos = 0;
	double max = 0.0;

	for( i = 0; i < cindex; i++ )
	{
		if( candidate[i].fre > max )
		{
			max = candidate[i].fre;
			max_pos = i;
		}
	}
	hps_debug(printf("candidate[%d] = %d\n", max_pos, max));
	if( max == 1 ) return -1;
	return max_pos;
}

int check_probability(  HPS *hps, char * in,  int row,int length, int *codes, int depth ,int *precond , ngram_t ngram )
{
	int	max_pos;
	int	cindex;
	candidate_t	candidate[256];
	memset(candidate, 0, sizeof(candidate_t)*256);
	cindex  = sum_frequency( hps, in , row, length, candidate , precond , ngram );
	max_pos = max_frequency( candidate, cindex );
	if( max_pos >= 0 ) 
	{
		hps_debug(printf("%s>>>>>>>>>>%d%s-%d-%d-%d\n\n",GREEN, candidate[max_pos].idx[0], RESET, candidate[max_pos].idx[1],candidate[max_pos].idx[2], candidate[max_pos].idx[3]));
		*precond  = candidate[max_pos].idx[0];
		memcpy( codes, candidate[max_pos].idx, sizeof(int)*MAX_SAVE_INDEX);
		return candidate[max_pos].idx[0];
	}
	else
	{
		hps_debug(printf("%s>>>>>>>>>>%d%s-%d-%d-%d\n\n",GREEN, 0, RESET,0,0,0));
		*precond  = 0;
		memset( codes, 0, sizeof(int)*MAX_SAVE_INDEX);
		return 0;
	}
}


void clear_summation( summation_t *summation )
{
	summation->cindex = 0;
}

space_ret_t	get_space( unsigned int code )
{
	switch( code )
	{
		case 0 :	return NO_NEED_SPACE;
		case 1 :	return NO_NEED_SPACE;
		case 4 :	return NO_NEED_SPACE;
		case 5 :	return NO_NEED_SPACE;
		case 2 :	return INSERT_SPACE;
		case 3 :	return INSERT_SPACE; 
		case 6 :	return INSERT_SPACE;
		case 7 :	return INSERT_SPACE;
	}
	return NO_NEED_SPACE;

}


void hps_clear_statistics( HPS *hps,int  row, int size )
{
    int i;
	for( i = row; i < size; i++ )
	{
		hps_statistics_push( hps, i, NULL );
	}
}

int	hps_only_statistics ( HPS *hps, char	*in, int size , table_t *table )
{
	int		i;
	int		row = 0;
	int		run = 0;
	int		epointer;
	int		termination_condition = 0;
	int		precond =  BASE_PRECOND;
	int		decision_depth = 3;
	int		codes[MAX_SAVE_INDEX] = { 0, };
	ngram_t	ngram;

	memset( hps->summation, 0, sizeof(summation_t) );
	hps_debug(printf("%s%s%s%s%s - %d\n\n",BOLD,ON_RED, in, RESET,RESET, size ));

	for( row = 0; row < size; )
	{
		ngram = TRIGRAM;
		termination_condition = TRI_TERMINATION_CONDITION;
		epointer = termination_condition + row;
		if((hps_load_ngram_statistics ( hps, in, row, termination_condition, ngram )) == -1)	
		{
			ngram = BIGRAM;
			termination_condition = BI_TERMINATION_CONDITION;
			epointer = termination_condition + row;

			
			if( size >= epointer )
			{
				if((hps_load_ngram_statistics ( hps, in, row, termination_condition, ngram )) == -1)	
				{
					hps_grammar_push( hps, row, row+1, get_statistics_tag( hps, row, NO_NEED_SPACE  ) ); 
					row += next_row( in, row );
					continue;
				}
			}
			else
			{
				hps_clear_statistics( hps, row, size );
			}
			hps_debug(printf("%s%s%s - %d : %d\n\n",ON_BLUE,"BIGRAM",RESET , row, row+termination_condition));
		}
		else
		{
			hps_debug(printf("%s%s%s - %d : %d \n\n",ON_RED,"TRIGRAM",RESET, row, row+termination_condition ));
		}
		if( !run || size >= epointer )//��?????? ????
		{
			switch( next_row( in, row ))
			{
				case 1 :
					switch( in[row] )
					{
						case ',' :
							hps_grammar_push( hps, row, row+1, get_statistics_tag( hps, row, INSERT_SPACE  ) ); 
							break;
						default :
							hps_grammar_push( hps, row, row+1, get_statistics_tag( hps, row, NO_NEED_SPACE  ) ); 
							break;
					}
					precond = BASE_PRECOND;
					row += 1;
					break;
				default :
					check_probability( hps,in, row, size , codes, decision_depth , &precond , ngram );
					run = 1;
					hps_grammar_push( hps, row, row+2, get_statistics_tag( hps, row, get_space( precond )  ) ); 
					clear_summation( hps->summation );
					row += 2;
					break;
			}

		}
		else//��??
		{
			for( i = 1; i < MAX_SAVE_INDEX; i++ )
			{
				if( codes[i] == 4 )
				{
					hps_grammar_push( hps, row,  row+4, get_statistics_tag( hps, row, INSERT_SPACE ) ); 
					row += next_row( in, row );
				}
				else
				{
					hps_grammar_push( hps, row,  row+4, get_statistics_tag( hps, row, get_space( codes[i]) ) ); 
				}
				row += next_row( in, row );
				if( row >= size )	break;
			}
			if( ( row+2 ) <= size )
				hps_grammar_push( hps, row, row+2, get_statistics_tag( hps, row, NO_NEED_SPACE  ) ); 
			return SUCC_STATICS;
		}
	}
	return SUCC_STATICS;
}

table_t* make_dic_table(  HPS *hps )
{
	int i;
	table_t *table;
	if((table = malloc(sizeof(table_t))) == NULL)
	{
		fprintf(stderr,"[%s:%d]\tcannot allocate memory \n",__FUNCTION__,__LINE__);
		return NULL;
	};
	table->TABLE = (char**)malloc(sizeof(char*)*MAX_STRLEN);
	for( i = 0; i < MAX_STRLEN; i++ )
	{
		if((table->TABLE[i] = malloc(sizeof(char)*MAX_STRLEN+1)) == NULL )
		{
			fprintf(stderr,"[%s:%d]\tcannot allocate memory \n",__FUNCTION__,__LINE__);
			return NULL;
		}

	}
	if((table->len   =  malloc(sizeof(short int)*MAX_STRLEN+1)) == NULL )
	{
			fprintf(stderr,"[%s:%d]\tcannot allocate memory \n",__FUNCTION__,__LINE__);
			return NULL;
	}
	return table;
}

void clr_dic_table ( HPS *hps, int size )
{
	int i, n;
	n = size <= 0 ? MAX_STRLEN:size;
	for( i = 0; i < n; i++ )
	memset( hps->table->TABLE[i],0,(n*sizeof(char)));
}

void free_dic_table( HPS *hps )
{
	int i;
	for( i = 0; i < MAX_STRLEN; i++ )
		free(hps->table->TABLE[i]);
	free(hps->table->TABLE);
	free(hps->table->len);
	free(hps->table);
}

void clr_position( HPS *hps )
{
	hps->position->index = 0;
	memset( hps->position->tag, 0, sizeof(char)*MAX_STACK );
	memset( hps->position->row, 0, sizeof(int)*MAX_STACK );
	memset( hps->position->col, 0, sizeof(int)*MAX_STACK );
	memset( hps->position->jos, 0, sizeof(int)*MAX_STACK );
}

position_t* make_position_( HPS *hps )
{
	position_t *position;
	position = malloc(sizeof(position_t));
	position->tag = malloc(sizeof(char)*MAX_STACK);
	position->row = malloc(sizeof(int)*MAX_STACK);
	position->col = malloc(sizeof(int)*MAX_STACK);
	position->jos = malloc(sizeof(int)*MAX_STACK);
	return position;
}

void free_position( HPS *hps )
{
	free( hps->position->tag );
	free( hps->position->row );
	free( hps->position->col );
	free( hps->position->jos );
	free( hps->position );
}


statistics_t* make_statistics( HPS * hps )
{
	int i;
	statistics_t *statistics;
	statistics = (statistics_t*)malloc(sizeof(statistics_t));
	statistics->mark       = malloc(sizeof(space_ret_t)*MAX_STACK);
	statistics->freq_array = malloc(sizeof(freq_array_t)*MAX_STACK);
	for( i = 0; i < MAX_STACK ; i++ )
		statistics->freq_array[i].frequency = malloc(sizeof(double)*MAX_STACK );
	return statistics;
}
void clr_statistics( HPS * hps )
{
	int i;
	hps->statistics->index = 0;
	memset( hps->statistics->mark, 0, sizeof(space_ret_t)*MAX_STACK);
	for( i = 0; i < MAX_STACK ; i++ )
		memset(hps->statistics->freq_array[i].frequency,0, sizeof(double)*MAX_STACK);

}
void free_statistics( HPS * hps )
{
	int i;
	for( i = 0; i < MAX_STACK ; i++ )
		free(hps->statistics->freq_array[i].frequency);
	free(hps->statistics->freq_array);
	free(hps->statistics->mark);
	free(hps->statistics );
}

/** @brief hps initialize function
  @param hps_dir HPS_HOME directory
  @param type input char type ( UTF | EUC )
 */
HPS*	hps_initialize ( char * hps_dir, euc_utf_t type , int max_thread )
{
	int		i;
	HPS		*hps_org;
	HPS		*hps;
	char	bi_tri[256];
	char	hps_tri[256];
	char	tri_tri[256];
	char	rep_tri[256];
	sprintf( bi_tri,   "%s/RunEnv/%s", hps_dir,BI_TRIE 	 );
	sprintf( tri_tri,  "%s/RunEnv/%s", hps_dir,TRI_TRIE  );
	sprintf( hps_tri,  "%s/RunEnv/%s", hps_dir,HPS_TRIE  );
	sprintf( rep_tri,  "%s/RunEnv/%s", hps_dir,REP_TRIE  );

	if( max_thread == 1 )
	{
		if( (hps = malloc(sizeof(HPS))) == NULL)                    fprintf(stderr, "Cannot allocate memory%s%s",       hps_tri, ENDLINE);


		if( (hps_trie_open( &hps->trie, hps_tri ))    == -1 )   fprintf(stderr, "Cannot load hps trie -- %s%s",     hps_tri,ENDLINE );
		if( (hps_trie_open( &hps->bigram, bi_tri ))   == -1 )   fprintf(stderr, "Cannot load bigram trie -- %s%s",  bi_tri,ENDLINE );
		if( (hps_trie_open( &hps->trigram, tri_tri )) == -1 )   fprintf(stderr, "Cannot load trigram trie -- %s%s", tri_tri,ENDLINE );
		if( (hps_trie_open( &hps->replace, rep_tri )) == -1 )	fprintf(stderr, "Cannot load replace trie -- %s%s", rep_tri,ENDLINE );

			hps->table      = make_dic_table( hps );
			hps->position   = make_position_( hps );
			hps->statistics = make_statistics( hps );
			hps->summation    = malloc(sizeof(summation_t));

			hps->blank   = 0;
			hps->preChar = 0;
			hps->HPS_DIC = 0;

			clr_position  ( hps );
			clr_dic_table ( hps, 0 );
			clr_statistics( hps ); 

	}
	else
	{

		if( (hps_org = malloc(sizeof(HPS))) == NULL)                fprintf(stderr, "Cannot allocate memory%s%s",       hps_tri, ENDLINE);
		if( (hps = malloc(sizeof(HPS)*max_thread)) == NULL)         fprintf(stderr, "Cannot allocate memory%s%s",       hps_tri, ENDLINE);


		if( (hps_trie_open( &hps_org->trie, hps_tri ))    == -1 )   fprintf(stderr, "Cannot load hps trie -- %s%s",     hps_tri,ENDLINE );
		if( (hps_trie_open( &hps_org->bigram, bi_tri ))   == -1 )   fprintf(stderr, "Cannot load bigram trie -- %s%s",  bi_tri,ENDLINE );
		if( (hps_trie_open( &hps_org->trigram, tri_tri )) == -1 )   fprintf(stderr, "Cannot load trigram trie -- %s%s", tri_tri,ENDLINE );
		if( (hps_trie_open( &hps_org->replace, rep_tri )) == -1 )	fprintf(stderr, "Cannot load replace trie -- %s%s", rep_tri,ENDLINE );

		for( i = 0; i < max_thread; i++ )
		{
			memcpy( &hps[i], hps_org, sizeof(HPS));

			hps[i].table      = make_dic_table( hps_org );
			hps[i].position   = make_position_( hps_org );
			hps[i].statistics = make_statistics( hps_org );
			hps[i].summation  = malloc(sizeof(summation_t));

			hps[i].blank   = 0;
			hps[i].preChar = 0;
			hps[i].HPS_DIC = 0;

			clr_position  ( &hps[i] );
			clr_dic_table ( &hps[i], 0 );
			clr_statistics( &hps[i] ); 

		}
		free( hps_org );
	}
	hps_init_grammar();
	hps_spacing = ( type == UTF ) ? hps_space_utf : hps_space_euc;
	switch( type ){
		case UTF : 	if( isatty(1) == 1 ) fprintf( stderr,"HPS running character set environment %s[UTF-8]%s MODE\n", GREEN, RESET );	break;
		default :	if( isatty(1) == 1 ) fprintf( stderr,"HPS running character set environment %s[EUCKR]%s MODE\n", GREEN, RESET );	break;}
					return hps;
}

int	hps_finalize( HPS *hps , int max_thread )
{
	int i;
	for( i = 0; i < max_thread; i++ )
	{
		if( &hps[i] != NULL )
		{
			free_position( &hps[i] );
			free_dic_table( &hps[i] );
			free_statistics( &hps[i] );
			free( hps[i].summation );
			if( i == 0 )
			{
				hps_close_trie( hps[i].trie		);
				hps_close_trie( hps[i].trigram	);
				hps_close_trie( hps[i].bigram	);
				hps_close_trie( hps[i].replace	);
			}
		}
	}
	free( hps );
	return 0;
}

int hps_increase_stack_pointer( int *sp )
{
	if( *sp >= (MAX_STRLEN-1) )
		return -1;
	else
		(*sp)++;
	return *sp;
}

int hps_decrease_stack_pointer( int *sp )
{
	if( *sp <= 0 )
		*sp = 0;
	else
		(*sp)--;
	return *sp;
}


int hps_check_dic ( HPS *hps, char  *in, SIZE_T size , table_t  *table )
{
	int		low_bound     =  0 ;
	int     sp            =  0 ;//stack pointer
	int     row           =  0 ;
	int     col           =  0 ;
	int     down          =  0 ;
	int     guess         =  0 ;
	int     pop_count     =  0 ;
	int     currenttag    = 'a';
	int     running_count =  0 ;
	char     tag_stack[MAX_STRLEN];

	if( hps->strong == 2 ) goto RETRY;

	tag_stack[sp] = 'o';
	if( hps_increase_stack_pointer( &sp ) == -1 ) return FAIL;

	if( *in & 0x80 ) low_bound = 1;

	for( row = 0; row < size - 1; row++ )
	{
		if( table->len[row] == 0 )  continue;
		for( col = row + table->len[row] - 1; col >= low_bound; col-- )
		{
			hps_debug(printf("sp = %d\n", sp ));
			currenttag = table->TABLE[row][col];

			if((running_count > 1000 && pop_count > 0) || (guess > 0 && hps->position->index == 0))
				goto RETRY;

			running_count++;

			if( currenttag > 0 )
			{
				if( sp > 0 && currenttag >= 'a' &&  grammar[tag_stack[sp-1]-'a'][currenttag-'a'] )
				{
					if( (char)currenttag == 'r' && down != row )
					{
						hps_grammar_push( hps, down, row, 'x' );
						hps_grammar_push( hps, row,  col, currenttag );
					}
					else
					{
						hps_grammar_push( hps, down, col, currenttag );
					}

					table->TABLE[row][col] *= -1;
					hps_debug(print_hps_check( hps, in , col,down, currenttag , PUSH ));
					tag_stack[sp] = currenttag;
					if( hps_increase_stack_pointer( &sp ) == -1 ) return FAIL;

					row  = col + 1;
					down = col + 1;
					if( row == size)    break;
					col = row + table->len[row];
				}
			}

			if( row == col )//cannot check
			{
				pop_count++;
				hps_decrease_stack_pointer( &sp );

				tag_stack[sp] = 0;
				if(hps_grammar_get( hps, &row, &col ) == 0)// if the first character
				{
					if( (HAN_MASK(in[row])))    row++;

					if(row == 0)    down = 0;
					else            down = row - 1;
					tag_stack[sp] = 'z';
					if( hps_increase_stack_pointer( &sp ) == -1 ) return FAIL;

					guess++;
				}
				else
				{
					down = row;
				}

				hps_debug(print_hps_check( hps, in, col, down, currenttag ,POP ));
				hps_grammar_pop( hps );
			}
		}
		if( sp == 1 && hps->size == 6 )
		{
			return FAIL;
		}
	}
	if( hps->position->index == 0 )
		goto RETRY;
	else
	{
		return SUCC;
	}

RETRY :

	if( hps->size == 6 ) return  FAIL;///3글자 띄어쓰기 오류일경우에는 사전만 사용하고 확률 정보는 사용하지 않는다
	running_count = 0;
	hps->position->index = 0;
	return hps_only_statistics ( hps, in, size , table );
	//return  FAIL;
}


int	hps_strlen ( HPS	*hps, char	*in)
{
	int	len = 0;
	char *pin;
	pin = in;
	while(*pin != '\0')
	{
		if(*pin == ' ')
		{
			while(1)
			{
				pin++;
				if(*pin == '\0')	break;
				if(*pin == ' ' )	pin++;
				else				len++;
			}
		}
		else
		{
			pin++;
			len++;
		}
	}
	return len;
}


int const MAX_HPS_CANDIDATE = 128;
extern int filter( char * strInputWord );

int hps_space_euc( HPS  *hps, char  *in, char   *out ,SIZE_T inputsize, SIZE_T outputsize )
{
	int	ret;
	*out = '\0';
	if( hps == NULL )return 0;

	if( 0 >= inputsize )                        return FAIL;
	if( inputsize >= MAX_HPS_CANDIDATE )        return FAIL;

	hps->size = inputsize;
	if( hps_load_dic        ( hps, in, inputsize, hps->table ))
	{
		ret = hps_check_dic   ( hps, in, inputsize, hps->table );
		if( ret != FAIL ){
			hps_make_word   ( hps, in, outputsize, out  );
			clr_dic_table ( hps, inputsize );
		}else{
			clr_dic_table ( hps, inputsize );
			hps_clear_statistics_info( hps , inputsize );;
			return FAIL;
		}
	}
	hps_clear_statistics_info( hps , inputsize );
	return hps->blank;
}

int hps_space_utf( HPS  *hps, char  *in, char   *out, SIZE_T inputsize, SIZE_T outputsize )
{
	int		ret;
	char    input[MAX_STACK];
	char    output[MAX_STACK];
	int utf2euc( char *utf, char * euc );
	int euc2utf( char *utf, char * euc );

	utf2euc( in, input );
	if( filter( in ) > 0)
		hps->blank++;


	inputsize = hps_strlen ( hps, input );
	if( 0 >= inputsize )                        return FAIL;
	if( inputsize >= MAX_HPS_CANDIDATE )        return FAIL;

	hps->size = inputsize;
	if( hps_load_dic        ( hps, input, inputsize, hps->table ))
	{
		ret = hps_check_dic   ( hps, input, inputsize, hps->table );
		if( ret != FAIL )
			hps_make_word   ( hps, input, outputsize, output );
		else
		{
			clr_dic_table ( hps, inputsize );
			hps_clear_statistics_info( hps , inputsize );
			return FAIL;
		}
	}
	hps_clear_statistics_info( hps , inputsize );
	{
		euc2utf( output, out );
		return hps->blank;
	}
	return hps->blank;
}



#define	HPS_MIN_LEN	10
#define	HPS_MAX_LEN	64
int	hps_spacing_only_space( HPS	*hps, char	*input, char	*output ,SIZE_T inputsize, SIZE_T outputsize )
{
	char		*token = NULL;
	char		temp[1024];
	int url_filter( char * string );
	*output = '\0';
	hps->position->index = 0;

	memset( output, 0, outputsize );
	strcpy(temp, input);
	token = strstr(input," ");
	if( url_filter( input ) == 1 )
	{
		strcpy(output,input);
		return SUCC;
	}
	if( filter( input ) > 0 )
	{
		strcpy(output,temp);
		return SUCC;
	} 

	hps->size = strlen(input);

	if(token == NULL && inputsize >= HPS_MIN_LEN && inputsize < HPS_MAX_LEN)
	{
		if( input[0] == '@' )
		{
			strcpy(output,input);
			return SUCC;
		}
		if( inputsize == 12 )//"XX?߾ƹ???"?? ????? ???? ?ʱ? ��??
		{
			if( strstr( input, "??" ) != NULL || strstr( input, "??" ) != NULL)
			{
				strcpy(output,input);
				return SUCC;
			}	
		}

		inputsize = hps_strlen ( hps, input );
		if( 0 >= inputsize )                        return FAIL;
		if( inputsize >= MAX_HPS_CANDIDATE )        return FAIL;

		hps->size = outputsize;
		if( hps_load_dic        ( hps, input, inputsize, hps->table )){
			if( hps_check_dic   ( hps, input, inputsize, hps->table ))
				hps_make_word   ( hps, input, outputsize, output    );
			else
			{
				hps_clear_statistics_info( hps , inputsize );
				return FAIL;
			}
		}
		ArrangeBlank(output);


		if(strcmp(input,output) != 0)
			fprintf(stderr,"HPS2.0[%s:%d]\t%20s\t\t[%s]\n",__FUNCTION__,__LINE__,input,output);
		hps_clear_statistics_info( hps , inputsize );
	}
	else
	{
		strcpy(output,input);
		hps_clear_statistics_info( hps , inputsize );;
	}
	return SUCC;
}


int	hps_spacing_only_space_for_total_utf8( HPS	*hps, char	*in, char *coll, char	*out,SIZE_T inputsize, SIZE_T outputsize, int debug )
{
	char	*token = NULL;
	char	temp[1024];
	char 	input[1024];
	char	output[1024];
	int ret;
	int utf2euc( char *utf, char * euc );
	int euc2utf( char *utf, char * euc );
	int url_filter( char * string );
	int has_result( char* , char *);
	int check_pattern( char * input, char *out,  char * pattern , func_t func , int );
	*output = '\0';
	hps->position->index = 0;


	utf2euc( in, input );


	strcpy(temp, input);
	token = strstr(input," ");
	if( url_filter( input ) == 1 )
	{
		if( check_pattern( input, output, "www.naver", refine_url , 1) == 1 )
			return SUCC;
		if( check_pattern( input, output, "www.facebook", refine_url , 1) == 1 )
			return SUCC;
		if( check_pattern( input, output, "www.twitter", refine_url , 1) == 1 )
			return SUCC;


		strcpy(output,input);
		return SUCC;
	}
	if( filter( input ) > 0 )
	{
		strcpy(output,temp);
		return SUCC;
	} 

	hps->size = strlen(input);

	if(token == NULL && inputsize >= HPS_MIN_LEN && inputsize < HPS_MAX_LEN)
	{
		if( input[0] == '@' )
		{
			strcpy(output,input);
			return SUCC;
		}
		if( inputsize == 12 )////"XX중아무개"를 띄어쓰기 하지 않기 위해
		{
			if( strstr( input, EUCKR_MIDDLE ) != NULL || strstr( input, EUCKR_HIGH ) != NULL)
			{
				strcpy(output,input);
				return SUCC;
			}	
		}
		if( check_pattern( input, output, EUCKR_SEASON, insert_forceful_space , 0) == 1 )
		{
			if( debug )	fprintf( stderr, "[%s:%d]\t(%s)-->(%s)\n", __FUNCTION__,__LINE__, input, output );
			if((ret = has_result( output , coll )) == 1 )
				return SUCC;
			else
				return FAIL;
		}

		if( check_pattern( input, output, "season", insert_forceful_space , 0) == 1 )
		{
			if( debug )	fprintf( stderr, "[%s:%d]\t(%s)-->(%s)\n", __FUNCTION__,__LINE__, input, output );
			if((ret = has_result( output , coll )) == 1 )
				return SUCC;
			else
				return FAIL;
		}

		inputsize = hps_strlen ( hps, input );
		if( 0 >= inputsize )                        return FAIL;
		if( inputsize >= MAX_HPS_CANDIDATE )        return FAIL;

		hps->size = outputsize;
		if( hps_load_dic        ( hps, input, inputsize, hps->table )){
			if( hps_check_dic   ( hps, input, inputsize, hps->table ))
				hps_make_word   ( hps, input, outputsize, output    );
			else
			{
				hps_clear_statistics_info( hps , inputsize );
				return FAIL;
			}
		}
		ArrangeBlank(output);
		if( strcmp( input, output ) != 0 )
		{
			ret = has_result( output , coll );
			if( ret == 1 )
			{
				fprintf(stderr,"[%s : %d] (%s)(%s)(%s) - SUCC : query change\n",__FUNCTION__, __LINE__ , input, output, coll); 
			}
			else
			{
				/*switch( ret )
				{
					case -1 :fprintf(stderr,"[%s : %d] (%s)(%s) - ERROR : inect_pton\n", __FUNCTION__, __LINE__, input, output ); break;
					case -2 :fprintf(stderr,"[%s : %d] (%s)(%s) - ERROR : connect\n",__FUNCTION__, __LINE__ , input, output); break;
					case -3 :fprintf(stderr,"[%s : %d] (%s)(%s) - ERROR : no result(NNN1)\n",__FUNCTION__, __LINE__ , input, output); break;
					case -4 :fprintf(stderr,"[%s : %d] (%s)(%s) - ERROR : no result(HSJ1_1TH1_RSJ1)\n",__FUNCTION__, __LINE__ , input, output); break;
					case -5 :fprintf(stderr,"[%s : %d] (%s)(%s) - ERROR : no result(?????? ???��ϴ?)\n",__FUNCTION__, __LINE__ , input, output); break;
					case -6 :fprintf(stderr,"[%s : %d] (%s)(%s) - ERROR : default return\n",__FUNCTION__, __LINE__ , input, output); break;
				}*/
				//fprintf(stderr,"[%s : %d] (%s)(%s) - FAIL : ret = %d\n",__FUNCTION__, __LINE__ , input, output, ret); 
				strcpy( output, input );
			}
		}

		hps_clear_statistics_info( hps , inputsize );
	}
	else
	{
		strcpy(output,input);
	}
	euc2utf( output/*euc*/, out/*utf*/ );
	return SUCC;
}


int	hps_spacing_only_space_for_total( HPS	*hps, char	*input, char *coll, char	*output ,SIZE_T inputsize, SIZE_T outputsize, int debug )
{
	char	*token = NULL;
	char	temp[1024];
	int ret;
	int url_filter( char * string );
	int has_result( char* , char *);
	int check_pattern( char * input, char *out,  char * pattern , func_t func , int );
	*output = '\0';
	hps->position->index = 0;



	strcpy(temp, input);
	token = strstr(input," ");
	if( url_filter( input ) == 1 )
	{
		if( check_pattern( input, output, "www.naver", refine_url , 1) == 1 )
			return SUCC;
		if( check_pattern( input, output, "www.facebook", refine_url , 1) == 1 )
			return SUCC;
		if( check_pattern( input, output, "www.twitter", refine_url , 1) == 1 )
			return SUCC;


		strcpy(output,input);
		return SUCC;
	}
	if( filter( input ) > 0 )
	{
		strcpy(output,temp);
		return SUCC;
	} 

	hps->size = strlen(input);

	if(token == NULL && inputsize >= HPS_MIN_LEN && inputsize < HPS_MAX_LEN)
	{
		if( input[0] == '@' )
		{
			strcpy(output,input);
			return SUCC;
		}
		if( inputsize == 12 )//"XX?߾ƹ???"?? ????? ???? ?ʱ? ��??
		{
			if( strstr( input, "??" ) != NULL || strstr( input, "??" ) != NULL)
			{
				strcpy(output,input);
				return SUCC;
			}	
		}
		if( check_pattern( input, output, "????", insert_forceful_space , 0) == 1 )
		{
			if( debug )	fprintf( stderr, "[%s:%d]\t(%s)-->(%s)\n", __FUNCTION__,__LINE__, input, output );
			if((ret = has_result( output , coll )) == 1 )
				return SUCC;
			else
				return FAIL;
		}

		if( check_pattern( input, output, "season", insert_forceful_space , 0) == 1 )
		{
			if( debug )	fprintf( stderr, "[%s:%d]\t(%s)-->(%s)\n", __FUNCTION__,__LINE__, input, output );
			if((ret = has_result( output , coll )) == 1 )
				return SUCC;
			else
				return FAIL;
		}

		inputsize = hps_strlen ( hps, input );
		if( 0 >= inputsize )                        return FAIL;
		if( inputsize >= MAX_HPS_CANDIDATE )        return FAIL;

		hps->size = outputsize;
		if( hps_load_dic        ( hps, input, inputsize, hps->table )){
			if( hps_check_dic   ( hps, input, inputsize, hps->table ))
				hps_make_word   ( hps, input, outputsize, output    );
			else
			{
				hps_clear_statistics_info( hps , inputsize );
				return FAIL;
			}
		}
		ArrangeBlank(output);
		if( strcmp( input, output ) != 0 )
		{
			ret = has_result( output , coll );
			if( ret == 1 )
			{
				fprintf(stderr,"[%s : %d] (%s)(%s)(%s) - SUCC : query change\n",__FUNCTION__, __LINE__ , input, output, coll); 
			}
			else
			{
				/*switch( ret )
				{
					case -1 :fprintf(stderr,"[%s : %d] (%s)(%s) - ERROR : inect_pton\n", __FUNCTION__, __LINE__, input, output ); break;
					case -2 :fprintf(stderr,"[%s : %d] (%s)(%s) - ERROR : connect\n",__FUNCTION__, __LINE__ , input, output); break;
					case -3 :fprintf(stderr,"[%s : %d] (%s)(%s) - ERROR : no result(NNN1)\n",__FUNCTION__, __LINE__ , input, output); break;
					case -4 :fprintf(stderr,"[%s : %d] (%s)(%s) - ERROR : no result(HSJ1_1TH1_RSJ1)\n",__FUNCTION__, __LINE__ , input, output); break;
					case -5 :fprintf(stderr,"[%s : %d] (%s)(%s) - ERROR : no result(?????? ???��ϴ?)\n",__FUNCTION__, __LINE__ , input, output); break;
					case -6 :fprintf(stderr,"[%s : %d] (%s)(%s) - ERROR : default return\n",__FUNCTION__, __LINE__ , input, output); break;
				}*/
				//fprintf(stderr,"[%s : %d] (%s)(%s) - FAIL : ret = %d\n",__FUNCTION__, __LINE__ , input, output, ret); 
				strcpy( output, input );
			}
		}

		hps_clear_statistics_info( hps , inputsize );
	}
	else
	{
		strcpy(output,input);
	}
	return SUCC;
}
