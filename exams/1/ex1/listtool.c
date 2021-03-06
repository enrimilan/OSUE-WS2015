#include <stdio.h>
#include <stdlib.h>             /* EXIT_FAILURE and EXIT_SUCCESS */
#include <assert.h>             /* assert() */
#include <string.h>
#include "list.h"

#define STREQ(a,b) (strcmp((a), (b)) == 0)

/* globals */
static const char *command = "<not yet set>";

/* prototypes for used functions */
static void usage(void);

/* implementation for challenge 2 */
static void insert_after(struct listelem *after, const char *const value)
{
    /* TODO: insert your code */
    /* when setting 'val' of the list, use strdup(value); */
    /* if you do not use strdup(), destroy() will fail/crash */
    struct listelem *new = malloc(sizeof(struct listelem));
    new -> val = strdup(value);
    new -> next = after -> next;
    after -> next = new;
    return;
}

int main(int argc, char **argv)
{
    /* do not touch */
    struct listelem *head;
    struct listelem *current;

    command = argv[0];
    head = init_list("head");
    populate_list(head);
    current = head;
    /* end do not touch */

    int opt;
    int opt_s = -1, opt_a = -1;
    char *optstr;               /* used to point to <string> */
    char *endptr;

    /* START OF INTENTIONAL ERRORS */
    while ((opt = getopt(argc, argv, "sa:")) != -1) {
        switch (opt) {
        	case 's':
            	if (opt_s != -1) {
            	    fprintf(stderr, "opt_s multiple times\n");
            	    usage();         /* does not return */
            	}
            	opt_s = 1;
            	break;
        	case 'a':
            	if (opt_a != -1) {
            	    fprintf(stderr, "opt_a multiple times\n");
                	usage();        /* does not return */
            	}
            	opt_a = strtol(optarg, &endptr, 10); /* strtol return checking not required */
            	break;
            case '?':
            	usage();
            	break;

            default:
            break;
        }
    }
    if(opt_s != -1 && opt_a != -1){
		usage();
    }
    if(opt_s == -1 && opt_a == -1){
		usage();
    }
    if(optind<argc){
		optstr = argv[optind];
    }
    else{
		usage();
    }
    
    
    /* END OF INTENTIONAL ERRORS */

    /* CHALLENGE 0:
     * 1) find the intentional failures in the marked section
     * 2) make program synopsis conformant, especially implement the following checks: */

    /* make sure that the user has not given both (-a and -s) options */
    /* make sure that the user has given at least one (-a or -s) option */
    /* point optstr to <string> (do not forget to check optind) */
    /* END OF CHALLENGE 0 */

    /* CHALLENGE 1:
     * iterate over the whole list and find the given <string>
     * remember: the 'next' pointer of the last list entry is set to NULL.
     * for every element print:
     * 'yes,' if it is equal to <string>, else 'no,' */
    if (opt_s != -1) {
        for (current = head; current != NULL; current = current->next ) {            /* TODO: change it */
            if (STREQ(optstr, current->val)) {  /* TODO: change it */
                printf("yes,");
            } else {
                printf("no,");
            }
        }
        printf("\n");           /* do not remove this line */
    }
    /* END OF CHALLENGE 1 */

    /* CHALLENGE 2:
     * insert the string <string> *after* the NUM-th element of the list
     * if NUM is larger than the number of list entries - 1, append the entry after the last element.
     * for debugging, the list can be printed with 'print_list(head)' */
    if (opt_a != -1) {
    		struct listelem *last;
    		int i = 0;
			/* iterate over the list and stop at the right entry */
        	for (current = head; current!=NULL; current = current -> next) {            /* TODO: change it */
				if(current!=NULL){
					last = current;
            	}
            	if(i==opt_a){
					break;
            	}
            	
            	i++;
        	}
        insert_after(last, optstr);  /* assuming you stopped at current */
        check_list(head);       /* do not remove this line or your solution does not count */
    }
    /* END OF CHALLENGE 2 */

    /* After all challenges, free the list */
    destroy_list(head);         /* do not remove this line */
    return 0;
}

static void usage(void)
{
    fprintf(stderr, "SYNOPSIS: %s [-s | -a NUM] <string>\n", command);
    fprintf(stderr,
            "\t-s and -a are only allowed once\n"
            "\tone of them has to be given\n"
            "\t<string> is mandatory\n"
            "\t-s iterates over the list and searches the string <string> in the list\n"
            "\t-a inserts the string <string> *after* the NUM-th element of the list\n"
            "\tif NUM is larger than the number of list entries - 1, append the entry after the last element\n"
            "\tfor debugging, the whole list can be printed with 'print_list' (cf., list.h)\n");

    fprintf(stderr,
            "EXAMPLES: (assume the list consists of the entries 'osue', 'test', 'ss2012')\n");
    fprintf(stderr, "\t%s => print usage\n", command);
    fprintf(stderr, "\t%s -s -s foo => print usage\n", command);
    fprintf(stderr, "\t%s foo => print usage\n", command);
    fprintf(stderr, "\t%s -s foo => no,no,no,\n", command);
    fprintf(stderr, "\t%s -s ss2012 => no,no,yes,\n", command);
    fprintf(stderr,
            "\t%s -a 0 foo => list becomes: osue, foo, test, ss2012\n",
            command);
    fprintf(stderr,
            "\t%s -a 2 foo => list becomes: osue, test, ss2012, foo\n",
            command);
    fprintf(stderr,
            "\t%s -a 23 foo => list becomes: osue, test, ss2012, foo\n",
            command);

    exit(EXIT_FAILURE);
}
