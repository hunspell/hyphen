#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "hyphen.h"

#define BUFSIZE 1000

void help() {
    fprintf(stderr,"correct syntax is:\n"); 
    fprintf(stderr,"example [-d | -dd] hyphen_dictionary_file file_of_words_to_check\n");
    fprintf(stderr,"-o = use old algorithm (without non-standard hyphenation)\n");
    fprintf(stderr,"-d = hyphenation with listing of the possible hyphenations\n");
    fprintf(stderr,"-n = print hyphenation vector\n");
}

/* get a pointer to the nth 8-bit or UTF-8 character of the word */
char * hindex(char * word, int n, int utf8) {
    int j = 0;
    while (j < n) {
        j++;
        word++;
        while (utf8 && ((((unsigned char) *word) >> 6) == 2)) word++;
    }
    return word;
}

/* list possible hyphenations with -dd option (example for the usage of the hyphenate2() function) */
void single_hyphenations(char * word, char * hyphen, char ** rep, int * pos, int * cut, int utf8) {
    int i, k, j = 0;
    char r;
    for (i = 0; (i + 1) < strlen(word); i++) {
        if (utf8 && ((((unsigned char) word[i]) >> 6) == 2)) continue;
        if ((hyphen[j] & 1)) {
            if (rep && rep[j]) {
              k = hindex(word, j - pos[j] + 1, utf8) - word;
              r = word[k];
              word[k] = 0;
              printf(" - %s%s", word, rep[j]);
              word[k] = r;
              printf("%s\n", hindex(word + k, cut[j], utf8));
            } else {
              k = hindex(word, j + 1, utf8) - word;
              r = word[k];
              word[k] = 0;
              printf(" - %s=", word);
              word[k] = r;
              printf("%s\n", word + k);
            }
        }
        j++;
    }
}

int 
main(int argc, char** argv)
{

    HyphenDict *dict;
    int df;
    int wtc;
    FILE* wtclst;
    int k, n, i, j, c;
    char buf[BUFSIZE + 1];
    int  nHyphCount;
    char *hyphens;
    char *lcword;
    char *hyphword;
    char *hword;
    int arg = 1;
    int optd = 1;
    int optn = 0;
    int optdd = 0;
    char ** rep;
    int * pos;
    int * cut;

  /* first parse the command line options */
  /* arg1 - hyphen dictionary file, arg2 - file of words to check */

  if (argv[arg]) {
       if (strcmp(argv[arg], "-o") == 0) {
            optd = 0;
            arg++;
       }
       if (strcmp(argv[arg], "-n") == 0) {
            optn = 1;
            arg++;
       }
       if (argv[arg] && strcmp(argv[arg], "-d") == 0) {
            optd = 1;
            optdd = 1;
            arg++;
       }
  }

  if (argv[arg]) {
       df = arg++;
  } else {
    help();
    exit(1);
  }

  if (argv[arg]) {
       wtc = arg++;
  } else {
    help();
    exit(1);
  }

  /* load the hyphenation dictionary */  
  if ((dict = hnj_hyphen_load(argv[df])) == NULL) {
       fprintf(stderr, "Couldn't find file %s\n", argv[df]);
       fflush(stderr);
       exit(1);
  }

  /* open the words to check list */
  wtclst = fopen(argv[wtc],"r");
  if (!wtclst) {
    fprintf(stderr,"Error - could not open file of words to check\n");
    exit(1);
  }

    
  /* now read each word from the wtc file */
    while(fgets(buf,BUFSIZE,wtclst) != NULL) {
       k = strlen(buf);
       if (k && buf[k - 1] == '\n') buf[k - 1] = '\0';
       if (k >=2 && buf[k - 2] == '\r') buf[k-- - 2] = '\0';

       /* set aside some buffers to hold lower cased */
       /* and hyphen information */
       lcword = (char *) malloc(k+1);
       hyphens = (char *)malloc(k+5);
       /* basic ascii lower-case, not suitable for real-world usage*/
       for (i = 0; i < k; ++i) {
         lcword[i] = buf[i];
         if ( (lcword[i] >= 'A') && (lcword[i] <= 'Z') )
           lcword[i] += 32;
       }

       /* first remove any trailing periods */
       n = k-1;
       while((n >=0) && (lcword[n] == '.')) n--;
       n++;

       /* now actually try to hyphenate the word */
       
       rep = NULL;
       pos = NULL;
       cut = NULL;
        
       /* set minimum required output buffer size (2 * word_size) */
       hword = (char *) malloc((n-1)*2);
       hword[0] = '\0';

       if ((!optd && hnj_hyphen_hyphenate(dict, lcword, n-1, hyphens)) ||
	    (optd && hnj_hyphen_hyphenate2(dict, lcword, n-1, hyphens, hword, &rep, &pos, &cut))) {
             free(hyphens);
             free(lcword);
             free(hword);
             fprintf(stderr, "hyphenation error\n");
             exit(1);
       }

       if (optn) fprintf(stderr, "%s\n", hyphens); 

       if (!optd) {
         /* now backfill hyphens[] for any removed periods */
         for (c = n; c < k; c++) hyphens[c] = '0';
         hyphens[k] = '\0';

         /* now create a new char string showing hyphenation positions */
         /* count the hyphens and allocate space for the new hypehanted string */
         nHyphCount = 0;
         for (i = 0; i < n; i++)
           if (hyphens[i]&1)
             nHyphCount++;
         hyphword = (char *) malloc(k+1+nHyphCount);
         j = 0;
         for (i = 0; i < n; i++) {
	   hyphword[j++] = buf[i];
           if (hyphens[i]&1) {
	      hyphword[j++] = '-';
	   }
         }
         hyphword[j] = '\0';
         fprintf(stdout,"%s\n",hyphword);
         fflush(stdout);
         free(hyphword);
      } else {
         fprintf(stdout,"%s\n", hword);


         if (optdd) single_hyphenations(lcword, hyphens, rep, pos, cut, dict->utf8);
         if (rep) {
            for (i = 0; i < n - 1; i++) {
                if (rep[i]) free(rep[i]);
            }
            free(rep);
            free(pos);
            free(cut);
         }
      }
      free(hyphens);
      free(lcword);
      free(hword);
    }

    fclose(wtclst);
    hnj_hyphen_free(dict);
    return 0;
}
