#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//src	tar	in	out	weight
//0		1	의	의	int
typedef struct record{
	int src;
	int tar;
	char in[8];
	char out[8];
	int count;
	double prob;
}record_t;


int c_utflen( char charUTF ){

        if(( charUTF & 0x80 ) == 0x00 ){
            return 1;
        } else if(( charUTF & 0xE0 ) == 0xC0 ){
            return 2;
        } else if(( charUTF & 0xF0 ) == 0xE0 ){
            return 3;
        } else if(( charUTF & 0xF8 ) == 0xF0 ){
            return 3;
        } else if(( charUTF & 0xFC ) == 0xF8 ){
            return 4;
        } else if(( charUTF & 0xFE ) == 0xFC ){
            return 5;
        }
        return -1;
}

	
int calculate_prob( record_t* record, int line ){

	int total, s = 0, e, i, j;
	total = record[i].count;
	for( i = 1; i < line; i++ ){
		if( record[i-1].src == record[i].src ){
			total += record[i].count;
		} else {
			e = i;
			for( j = s; j < e; j++ ){
				record[s].prob = (total - record[j].count)/(double)total;
			}
			s = i;
		}

	}
}

int main( int argc, char *argv[] ){

	FILE *fp1, *fp2, *fw;
	int *idx, line = 0, i, found;
	int curr_idx, count, pos, len, bsrc=0;
	char buffer[1024],*p, *token, *bp;

	if( argc != 4 ){
		fprintf( stderr,"%s <att file> <uni file> <wfst file>\n", argv[0] );
		return 0;
	}
	
	record_t *record;
	record = malloc( sizeof(record_t)*60);
	idx    = malloc( sizeof(int)*60 );
	for( i = 0; i < 60; i++ ) idx[i] = 0;


	if((fp1 = fopen( argv[1], "r")) == NULL ){
		fprintf( stderr,"cannot open file = %s\n", argv[1] );
		return 0;
	}
	while( fgets( buffer, 1024, fp1 ) != NULL ){ if(( p = strchr( buffer, '\n' )) != NULL ) *p = '\0';

		for( token = strtok_r( buffer, "\t", &bp ), i=0; token != NULL; token = strtok_r(NULL, "\t", &bp ), i++){
			switch( i ){
				case 0 : record[line].src = atoi( token ); break;
				case 1 : record[line].tar = atoi( token ); break;
				case 2 : if( strcmp(token, "@0@" ) == 0 ) { strcpy( record[line].in, "<eps>" ); } else { strcpy(record[line].in,  token );} break;
				case 3 : strcpy(record[line].out, token ); break;
				default : break;
			}
		}
		if( bsrc != record[line].src ){
			idx[record[line].src] = line;
			bsrc = record[line].src;
		}
		line++;
	}

	fclose(fp1);
	if((fp2 = fopen( argv[2], "r")) == NULL ){
		fprintf( stderr,"cannot open file = %s\n", argv[2] );
		return 0;
	}
	while( fgets( buffer, 1024, fp2 ) != NULL ){ if(( p = strchr( buffer, '\n' )) != NULL ) *p = '\0';

		if(( token = strtok_r( buffer, "\t" , &bp )) != NULL )	count = atoi(bp);
		
		pos = 0;
		curr_idx = 0;
		for( pos = 0; pos < strlen(buffer); pos += len ){
			len = c_utflen(buffer[pos]);
			found = 0;

			if( idx[curr_idx] == 0 && curr_idx != 0 ){
				if(( strncmp( record[curr_idx].in, &buffer[pos], len )) == 0 ){
					record[curr_idx].count += count;
					curr_idx = idx[record[curr_idx].tar];
					found = 1;
					break;
				}
			} else {
				for( i = 0; i < (idx[curr_idx+1]-idx[curr_idx]); i++ ){
					if(( strncmp( record[i+curr_idx].in, &buffer[pos], len )) == 0 ){
						record[i+curr_idx].count += count;
						curr_idx = idx[record[i+curr_idx].tar];
						found = 1;
						break;
					}
				}
			}
			if( !found ) break;
		}

		for( i = 0;  ; i++ ){
			if( record[curr_idx].src != record[i+curr_idx].src ) break;
			if( strcmp( record[i+curr_idx].in , "<eps>" ) == 0 ){
				record[i+curr_idx].count += count;
			}

		}
	}
	fclose(fp2);


	calculate_prob( record, line );


	if((fw = fopen( argv[3], "w")) == NULL ){
		fprintf( stderr,"cannot open file = %s\n", argv[3] );
		return 0;
	}

	for( i = 0; i < line; i++ ){
		if( record[i].in[0] != '\0' ){
		fprintf( fw, "%d\t%d\t%s\t%s\t%f\n", record[i].src, record[i].tar, record[i].in, record[i].out, record[i].prob );
		fprintf( stdout, "%d\t%d\t%s\t%s\t%f\n", record[i].src, record[i].tar, record[i].in, record[i].out, record[i].prob );
		} else {
		fprintf( fw, "%d\n", record[i].src );
		fprintf( stdout, "%d\n", record[i].src );
		}
	}



	fclose(fw);
	free( idx );
	free(record);
	return 0;
}
