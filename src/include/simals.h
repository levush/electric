/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: simals.h
 * Header file for asynchronous logic simulator
 * From algorithms by: Brent Serbin and Peter J. Gallant
 * Last maintained by: Steven M. Rubin
 *
 * Copyright (c) 2000 Static Free Software.
 *
 * Electric(tm) is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Electric(tm) is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Electric(tm); see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, Mass 02111-1307, USA.
 *
 * Static Free Software
 * 4119 Alpine Road
 * Portola Valley, California 94028
 * info@staticfreesoft.com
 */

#if defined(__cplusplus) && !defined(ALLCPLUSPLUS)
extern "C"
{
#endif

typedef enum {DELAY_MIN=0, DELAY_TYP, DELAY_MAX} DELAY_TYPES;

/* The trace buffer in ALS can currently hold 10000 events.
 * More events can be accommodated in the trace buffer by
 * increasing its size.
 */
#define DEFAULT_TRACE_SIZE	10000

typedef struct model_ds
{
	INTBIG              num;
	CHAR               *name;
	CHAR                type;
	CHAR               *ptr;	/* may be CONPTR or ROWPTR or FUNCPTR */
	struct export_ds   *exptr;
	struct io_ds       *setptr;
	struct load_ds     *loadptr;
	CHAR                fanout;
	INTSML              priority;
	struct model_ds    *next;
	CHAR               *level;  /* hierarchical level */
} MODEL;
typedef MODEL *MODPTR;

typedef struct row_ds
{
	struct io_ds   *inptr;
	struct io_ds   *outptr;
	float           delta;
	float           linear;
	float           exp;
	float           random;
	float           abs;    /* BA delay - SDF absolute port delay */
	struct row_ds  *next;
	CHAR           *delay;  /* delay transition name (01, 10, etc) */
} ROW;
typedef ROW *ROWPTR;

typedef struct io_ds
{
	struct node_ds *nodeptr;
	UCHAR           operatr;
	CHAR           *operand;
	INTSML          strength;
	struct io_ds   *next;
} IO;
typedef IO *IOPTR;

typedef struct connect_ds
{
	CHAR               *inst_name;
	CHAR               *model_name;
	struct export_ds   *exptr;
	struct connect_ds  *parent;
	struct connect_ds  *child;
	struct connect_ds  *next;
	struct channel_ds  *display_page;  /* pointer to the display page */
	INTBIG              num_chn;       /* number of exported channels in this level */
} CONNECT;
typedef CONNECT *CONPTR;

typedef struct export_ds
{
	CHAR              *node_name;
	struct node_ds    *nodeptr;
	struct export_ds  *next;
	INTBIG            td[12];  /* transition delays */
} EXPORT;
typedef EXPORT *EXPTR;

typedef struct load_ds
{
	CHAR            *ptr;
	float            load;
	struct load_ds  *next;
} LOAD;
typedef LOAD *LOADPTR;

typedef struct func_ds
{
	void             (*procptr)(MODPTR);
	struct export_ds  *inptr;
	float              delta;
	float              linear;
	float              exp;
	float              abs;    /* absolute delay for back annotation */
	float              random;
	CHAR              *userptr;
	INTBIG             userint;
	float              userfloat;
} FUNC;
typedef FUNC *FUNCPTR;

typedef struct node_ds
{
	struct connect_ds  *cellptr;
	INTBIG              num;
	INTBIG              sum_state;
	INTSML              sum_strength;
	INTBIG              new_state;
	INTSML              new_strength;
	BOOLEAN             tracenode;
	INTBIG              plot_node;
	struct stat_ds     *statptr;
	struct load_ds     *pinptr;
	float               load;
	INTBIG              visit;
	INTBIG              maxsize;
	INTBIG              arrive;
	INTBIG              depart;
	float               tk_sec;
	double              t_last;
	struct node_ds     *next;
} NODE;
typedef NODE *NODEPTR;

typedef struct stat_ds
{
	struct model_ds  *primptr;
	struct node_ds   *nodeptr;
	INTBIG            new_state;
	INTSML            new_strength;
	UCHAR             sched_op;
	INTBIG            sched_state;
	INTSML            sched_strength;
	struct stat_ds   *next;
} STAT;
typedef STAT *STATPTR;

typedef struct link_ds
{
	struct link_ds  *left;
	struct link_ds  *right;
	struct link_ds  *up;
	struct link_ds  *down;
	CHAR            *ptr;
	CHAR             type;
	UCHAR            operatr;
	INTBIG           state;
	INTSML           strength;
	INTSML           priority;
	double           time;
	MODPTR           primhead;
} LINK;
typedef LINK *LINKPTR;

typedef struct trak_ds
{
	struct node_ds  *ptr;
	INTBIG           state;
	INTSML           strength;
	double           time;
} TRAK;
typedef TRAK *TRAKPTR;

typedef struct channel_ds
{
	CHAR            *name;
	struct node_ds  *nodeptr;
	INTBIG           displayptr;
} CHANNEL;
typedef CHANNEL *CHNPTR;

/*
 * Now come all the global variables declared extern for reference by other files
 */
extern MODPTR      simals_modroot, simals_primroot;
extern ROWPTR      simals_rowptr2;
extern IOPTR       simals_ioptr2;
extern CONPTR      simals_levelptr, simals_cellroot;
extern EXPTR       simals_exptr2;
extern NODEPTR     simals_noderoot, simals_drive_node;
extern LINKPTR     simals_linkfront, simals_linkback, simals_setroot;
extern TRAKPTR     simals_trakroot;
extern LOADPTR     simals_chekroot;
extern CHAR      **simals_rowptr1, **simals_ioptr1;
extern CHAR       *simals_instbuf;
extern INTBIG      simals_pseq, simals_nseq, *simals_instptr,
				   simals_trakfull, simals_trakptr;
extern INTBIG      simals_no_update_key;		/* variable key for "SIM_als_no_update" */
extern BOOLEAN     simals_seed_flag, simals_trace_all_nodes;
extern double      simals_time_abs;
extern NODEPROTO  *simals_mainproto;
extern INTBIG      simals_trace_size;
extern CHAR       *simals_title;

/* prototypes for intratool interface */
LINKPTR   simals_alloc_link_mem(void);
#ifdef DEBUGMEMORY
  CHAR   *_simals_alloc_mem(INTBIG, CHAR*, INTBIG);
# define  simals_alloc_mem(a) _simals_alloc_mem((a), (CHAR *)__FILE__, (INTBIG)__LINE__)
#else
  CHAR   *simals_alloc_mem(INTBIG);
#endif
void      simals_annotate_command(INTBIG, CHAR*[]);
INTBIG    simals_atoi(CHAR*);
void      simals_build_actel_command(INTBIG, CHAR*[]);
void      simals_clearallvectors(BOOLEAN);
void      simals_clock_command(INTBIG, CHAR*[]);
void      simals_com_comp(INTBIG, CHAR*[10]);
void      simals_compute_node_name(NODEPTR, CHAR*);
CHAR     *simals_compute_path_name(CONPTR);
void      simals_convert_to_upper(CHAR*);
void      simals_erase_model(void);
void      simals_fill_display_arrays(void);
CONPTR    simals_find_level(CHAR*);
NODEPTR   simals_find_node(CHAR*);
BOOLEAN   simals_flatten_network(void);
BOOLEAN   simals_fragment_command(CHAR*);
void      simals_free_link_mem(LINKPTR);
void      simals_freeflatmemory(void);
void      simals_freesimmemory(void);
INTBIG   *simals_get_function_address(CHAR*);
void      simals_go_command(INTBIG, CHAR*[]);
void      simals_help_command(void);
void      simals_init(void);
void      simals_init_display(void);
double    simals_initialize_simulator(BOOLEAN);
void      simals_insert_link_list(LINKPTR);
CHAR    **simals_insert_set_list(LINKPTR);
void      simals_level_up_command(void);
void      simals_level_set_command(CHAR *instname);
CHAR     *simals_nextinstance(void);
void      simals_order_command(INTBIG, CHAR*[]);
void      simals_print_command(INTBIG, CHAR*[]);
BOOLEAN   simals_read_net_desc(NODEPROTO*);
void      simals_seed_command(INTBIG, CHAR*[]);
void      simals_set_command(INTBIG, CHAR*[]);
BOOLEAN   simals_set_current_level(void);
void      simals_term(void);
BOOLEAN   simals_topofinstances(CHAR**);
void      simals_trace_command(INTBIG, CHAR*[]);
CHAR     *simals_trans_number_to_state(INTBIG);
INTBIG    simals_trans_state_to_number(CHAR*);
void      simals_vector_command(INTBIG, CHAR*[]);

BOOLEAN   simals_startsimulation(NODEPROTO *np);
BOOLEAN   simals_charhandlerschem(WINDOWPART*, INTSML, INTBIG);
BOOLEAN   simals_charhandlerwave(WINDOWPART*, INTSML, INTBIG);
void      simals_reportsignals(WINDOWPART *simwin, void *(*addbranch)(CHAR*, void*),
			void *(*findbranch)(CHAR*, void*), void *(*addleaf)(CHAR*, void*), CHAR *(*nodename)(void*));

#if defined(__cplusplus) && !defined(ALLCPLUSPLUS)
}
#endif
