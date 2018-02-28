kö#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <values.h>
#include <limits.h>

#define MAXOFFSET 1025
#define NAN FLT_MIN

typedef struct column_s {
  float cbuf[MAXOFFSET];
  int offset;
  int index;
  int pastHalfway;
} *column;

column initColumn(int offset) {
  column r = (column)calloc(sizeof(struct column_s),1);
  r->offset = offset;
  r->index = 0;
  r->pastHalfway = 0;
}

float getMidVal(column state) {
  if (!state->pastHalfway) {
    return NAN;
  } else {
    return state->cbuf[(state->index + state->offset / 2) % (state->offset)];
  }
}

void addValue(column state, float v) {
  state->cbuf[state->index] = v;
  state->index = (state->index + 1) % state->offset;
  if (state->index >= state->offset/2) {
    state->pastHalfway = 1;
  }
}

char **offsetbuf=NULL;
int offsetcounter=0;

#define NAN_int MAXINT
#define NAN_double MAXDOUBLE

void offsetprint(int offset, char *preamble, int v1, double v2) {
  if (!offsetbuf) offsetbuf=(char **)malloc(offset * sizeof(char *));
  char vbuf1[128];
  char vbuf2[128];
  char *outbuf;
  if (v1 == NAN_int) strcpy(vbuf1, "NaN");
  else sprintf(vbuf1, "%d", v1);
  outbuf=(char *)malloc((strlen(preamble) + strlen(vbuf1) + 1) * sizeof(char));
  sprintf(outbuf, "%s%s", preamble, vbuf1);
  if (offsetcounter < offset) {
    offsetbuf[offsetcounter++] = outbuf;
  } else {
    offsetbuf[offsetcounter] = outbuf;
    outbuf=offsetbuf[0];
    memmove(offsetbuf, &(offsetbuf[1]), offsetcounter * sizeof(char *)); // we use the last+1 position for the new value!
    if (v2 == NAN_double) strcpy(vbuf2, "NaN");
    else sprintf(vbuf2, "%.2f", v2);
    printf("%s,%s\n", outbuf, vbuf2);
    free(outbuf);
  }
}

void offsetfinish(int offset) {
  int i;
  if (offsetbuf == NULL) return;
  for (i=0; i<offsetcounter; i++) {
    printf("%s,NaN\n", offsetbuf[i]);
    free(offsetbuf[i]);
  }
}

typedef struct colno_s {
  int col;
  struct colno_s *next;
} colno;

void add_colno(colno **list, int val) {
  if (*list == NULL || (*list)->col < val) {
    colno *tmp = (colno*) malloc(sizeof(colno));
    tmp->next = *list;
    tmp->col = val;
    (*list) = tmp;
  } else {
    add_colno(&((*list)->next), val);
  }
}

void trim(char *s) {
  while (strlen(s) && isspace(s[strlen(s)-1])) s[strlen(s) - 1] = '\0';
}

int main(int argc, char *argv[]) {
  int avlen=30;
  colno *columns = NULL;
  char inbuf[8192];
  int inv, sum;
  double outv;
  int o;
  int center = 0;
  while ((o=getopt(argc, argv, "l:f:c"))!=-1) {
    switch (o) {
    case 'l':
      avlen = atoi(optarg);
      break;
    case 'f':
      char *s = optarg;
      char *c;
      while ((c = strtok(s, ",")) != NULL) {
	add_colno(&columns, atoi(c));
	s=NULL;
      }
      break;
    case 'c':
      center = 1;
      break;
    }
  }

  /*
   * TODO:
   * Read the first row, assuming it's a header row, and emit it, including made-up
   * headings for the new columns.
   * Read each row, extract the indicated columns and maintain moving averages for
   * each.
   * Produce a suffix containing the moving averages for each column as a comma-separated
   * string.
   * Send the current row and the suffix string into the offset handler, and let it queue
   * up either to handle the offset.
   */
  int offset=(int)(0.5 + (float)avlen / 2);
  int v[avlen];
  int i, filled=0;
  for (i=0; i<avlen; i++) v[i]=0;
  i=0;
  while (fgets(inbuf, sizeof(inbuf), stdin) != NULL) {
    trim(inbuf);
    char *p=strchr(inbuf, ',');
    if (p) {
      p++;
      if (isdigit(*p)) {
	inv = atoi(p);
	*p = '\0';
	v[i++]=inv;
	if (filled < avlen) filled++;
	i = i%avlen;
	sum += inv - v[i];
	outv = (double)sum / (double)filled;
	offsetprint(offset, inbuf, inv, outv);
      } else {
	offsetprint(offset, inbuf, NAN_int, NAN_double);
      }
    }
  }
  offsetfinish(offset);
  return 0;
}

