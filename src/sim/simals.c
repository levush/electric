/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: simals.c
 * Asynchronous Logic Simulator main module
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

#include "config.h"
#if SIMTOOL

#include "global.h"
#include "sim.h"
#include "simals.h"

/********************************* GLOBALS *********************************/

       MODPTR      simals_modroot = 0;
static MODPTR      simals_modptr2;
       MODPTR      simals_primroot = 0;
       ROWPTR      simals_rowptr2;
       IOPTR       simals_ioptr2;
static CONPTR      simals_conptr2;
       CONPTR      simals_levelptr = 0;
       CONPTR      simals_cellroot = 0;
       EXPTR       simals_exptr2;
       NODEPTR     simals_noderoot = 0;
       NODEPTR     simals_drive_node;
       LINKPTR     simals_linkfront = 0;
       LINKPTR     simals_linkback = 0;
       LINKPTR     simals_setroot = 0;
       TRAKPTR     simals_trakroot = 0;
       LOADPTR     simals_chekroot;
       CHAR      **simals_rowptr1;
       CHAR      **simals_ioptr1;
       CHAR       *simals_instbuf = 0;
static INTBIG      simals_loop_count;
       INTBIG      simals_pseq;
       INTBIG      simals_nseq;
       INTBIG     *simals_instptr = 0;
static INTBIG      simals_ibufsize;
static INTBIG      simals_iptrsize;
       INTBIG      simals_trakfull;
       INTBIG      simals_trakptr;
       INTBIG      simals_no_update_key;		/* variable key for "SIM_als_no_update" */
       BOOLEAN     simals_seed_flag = TRUE;
       BOOLEAN     simals_trace_all_nodes = FALSE;
       double      simals_time_abs;
static float       simals_delta_def;
static float       simals_linear_def;
static float       simals_exp_def;
static float       simals_random_def;
static float       simals_abs_def;
       NODEPROTO  *simals_mainproto = NONODEPROTO;
       INTBIG      simals_trace_size = 0;
       CHAR       *simals_title = 0;

/********************************* LOCALS *********************************/

static INTBIG     i_ptr;
static CHAR       delay[16];

/* prototypes for local routines */
static BOOLEAN simals_alloc_char_buffer(CHAR**, INTBIG*, INTBIG);
static BOOLEAN simals_get_string(CHAR*);
static BOOLEAN simals_get_int(INTBIG*);
static BOOLEAN simals_get_float(float*);
static BOOLEAN simals_get_name(CHAR*);
static BOOLEAN simals_fragment_line(CHAR*);
static BOOLEAN simals_parse_struct_header(CHAR);
static BOOLEAN simals_parse_gate(void);
static BOOLEAN simals_parse_node(void);
static BOOLEAN simals_parse_timing(void);
static BOOLEAN simals_parse_fanout(void);
static BOOLEAN simals_parse_load(void);
static BOOLEAN simals_parse_model(void);
static BOOLEAN simals_parse_function(void);
static BOOLEAN simals_parse_func_input(EXPORT**);
static BOOLEAN simals_parse_func_output(void);
static BOOLEAN simals_parse_delay(void);
static BOOLEAN simals_alloc_ptr_buffer(INTBIG**, INTBIG*, INTBIG);

/******************************************************************************/

void simals_init(void)
{
	REGISTER VARIABLE *var;

	/* allocate memory */
	if (simals_instbuf == 0)
	{
		if (simals_alloc_char_buffer(&simals_instbuf, &simals_ibufsize, 200)) return;
	}
	if (simals_instptr == 0)
	{
		if (simals_alloc_ptr_buffer(&simals_instptr, &simals_iptrsize, 100)) return;
	}
	if (simals_trace_size == 0)
	{
		var = getval((INTBIG)sim_tool, VTOOL, VINTEGER, x_("SIM_als_num_events"));
		if (var == NOVARIABLE) simals_trace_size = DEFAULT_TRACE_SIZE; else
			simals_trace_size = var->addr;
	}
	if (simals_trakroot == 0)
	{
		simals_trakroot = (TRAKPTR)simals_alloc_mem((INTBIG)(simals_trace_size * sizeof(TRAK)));
		if (simals_trakroot == 0) return;
	}
	simals_no_update_key = makekey(x_("SIM_als_no_update"));
}

void simals_term(void)
{
	simals_erase_model();
	if (simals_instbuf != 0) efree((CHAR *)simals_instbuf);
	if (simals_instptr != 0) efree((CHAR *)simals_instptr);
	if (simals_trakroot != 0) efree((CHAR *)simals_trakroot);
	simals_freeflatmemory();
	simals_freesimmemory();
}

/*
 * routine to create a new window with simulation of cell "simals_mainproto"
 */
void simals_init_display(void)
{
	CONPTR cellptr;
	CHAR *pt;
	REGISTER void *infstr;

	if (simals_mainproto == NONODEPROTO)
	{
		ttyputerr(_("No cell to simulate"));
		return;
	}

	/* set top level */
	simals_levelptr = simals_cellroot;
	infstr = initinfstr();
	addstringtoinfstr(infstr, simals_mainproto->protoname);
	pt = returninfstr(infstr);
	simals_convert_to_upper(pt);
	cellptr = simals_find_level(pt);
	if (cellptr != 0) simals_levelptr = cellptr;
	if (simals_levelptr == 0)
	{
		ttyputerr(_("No simulation to resume"));
		return;
	}
	if (simals_set_current_level()) return;

	/* run simulation */
	(void)simals_initialize_simulator(TRUE);
}

void simals_com_comp(INTBIG count, CHAR *par[10])
{
	CHAR *pp;
	INTBIG l;

	l = estrlen(pp = par[0]);

	if (namesamen(pp, x_("build-actel-models"), l) == 0 && l >= 1)
	{
		simals_build_actel_command(count-1, &par[1]);
		return;
	}
	if (namesamen(pp, x_("clock"), l) == 0 && l >= 1)
	{
		simals_clock_command(count-1, &par[1]);
		return;
	}
	if (namesamen(pp, x_("erase"), l) == 0 && l >= 1)
	{
		simals_erase_model();
		return;
	}
	if (namesamen(pp, x_("go"), l) == 0 && l >= 1)
	{
		simals_go_command(count-1, &par[1]);
		return;
	}
	if (namesamen(pp, x_("help"), l) == 0 && l >= 1)
	{
		simals_help_command();
		return;
	}
	if (namesamen(pp, x_("print"), l) == 0 && l >= 1)
	{
		simals_print_command(count-1, &par[1]);
		return;
	}
	if (namesamen(pp, x_("seed"), l) == 0 && l >= 3)
	{
		simals_seed_command(count-1, &par[1]);
		return;
	}
	if (namesamen(pp, x_("set"), l) == 0 && l >= 3)
	{
		simals_set_command(count-1, &par[1]);
		return;
	}
	if (namesamen(pp, x_("trace"), l) == 0 && l >= 1)
	{
		simals_trace_command(count-1, &par[1]);
		return;
	}
	if (namesamen(pp, x_("vector"), l) == 0 && l >= 1)
	{
		simals_vector_command(count-1, &par[1]);
		return;
	}
	if (namesamen(pp, x_("annotate"), l) == 0 && l >= 1)
	{
		simals_annotate_command(count-1, &par[1]);
		return;
	}
	if (namesamen(pp, x_("order"), l) == 0 && l >= 1)
	{
		simals_order_command(count-1, &par[1]);
		return;
	}
	ttyputbadusage(x_("telltool simulation als"));
}

/*
 * Name: simals_alloc_mem
 *
 * Description:
 *	This procedure allocates a block of memory of size that is defined
 * by the calling argument.  If there are memory allocation problems,
 * a diagnostic message is generated and program execution is terminated.
 * The procedure returns an integer value representing the address of the
 * newly allocated memory.  Returns zero on error.
 *
 * Calling Arguments:
 *	size = integer value representing size of data element
 */
#ifdef DEBUGMEMORY
CHAR *_simals_alloc_mem(INTBIG size, CHAR *filename, INTBIG lineno)
#else
CHAR *simals_alloc_mem(INTBIG size)
#endif
{
	CHAR   *ptr;
	INTBIG   i;

#ifdef DEBUGMEMORY
	ptr = (CHAR *)_emalloc(size, sim_tool->cluster, filename, lineno);
#else
	ptr = (CHAR *)emalloc(size, sim_tool->cluster);
#endif
	if (ptr == 0)
	{
		ttyputmsg(_("ERROR: Memory Allocation Error"));
		return(0);
	}
	for(i=0; i<size; i++) ptr[i] = 0;
	return(ptr);
}

/*
 * Name: simals_alloc_char_buffer
 *
 * Description:
 *	This procedure allocates a buffer to contain the specified number
 * of data elements for the text buffer.  If the buffer was already allocated
 * the contents of the old one are copied into the new buffer after the memory
 * has been allocated.  Returns true on error.
 *
 * Calling Arguments:
 *	root    = address of pointer to the beginning of character space
 *	old     = address of int value indicating the old size of the buffer
 *	newsize = integer value indicating the new size to be created
 */
BOOLEAN simals_alloc_char_buffer(CHAR **root, INTBIG *old, INTBIG newsize)
{
	INTBIG   i;
	CHAR  *head;

	head = (CHAR *) simals_alloc_mem((INTBIG)(newsize * sizeof(CHAR)));
	if (head == 0) return(TRUE);
	if (*root)
	{
		for (i = 0; i < (*old); ++i) head[i] = (*root)[i];
		efree((CHAR *)*root);
	}

	*root = head;
	*old = newsize;
	return(FALSE);
}

/*
 * Name: simals_alloc_ptr_buffer
 *
 * Description:
 *	This procedure allocates a buffer to contain the specified number
 * of data elements for the pointers to char strings in the char buffer.  If
 * the buffer was already allocated the contents of the old one are copied into
 * the new buffer after the memory has been allocated.  Returns true on error.
 *
 * Calling Arguments:
 *	root    = address of pointer to the beginning of pointer space
 *	old     = address of int value indicating the old size of the buffer
 *	newsize = integer value indicating the new size to be created
 */
BOOLEAN simals_alloc_ptr_buffer(INTBIG **root, INTBIG *old, INTBIG newsize)
{
	INTBIG   i, **head;

	head = (INTBIG **) simals_alloc_mem((INTBIG)(newsize * sizeof(INTBIG *)));
	if (head == 0) return(TRUE);

	if (*root)
	{
		for (i = 0; i < (*old); ++i) head[i] = (INTBIG*) (*root)[i];
		efree((CHAR *)*root);
	}

	*root = (INTBIG*) head;
	*old = newsize;
	return(FALSE);
}

/*
 * Name: simals_get_string
 *
 * Description:
 *	This procedure gets one string from the instruction buffer.  The
 * procedure returns a Null value if the string was successfully obtained.
 * A true value is returned when End Of File (EOF) is encountered.
 *
 * Calling Arguments:
 *	sp = pointer to char string where results are to be saved
 */
BOOLEAN simals_get_string(CHAR *sp)
{
	INTBIG   i;
	CHAR  line[260];
	extern TOOL *vhdl_tool;

	if (((simals_instptr[i_ptr] < 0) || (simals_instbuf[simals_instptr[i_ptr]] == '#')) && simals_loop_count)
	{
		++i_ptr;
		return(TRUE);
	}

	while ((simals_instptr[i_ptr] < 0) || (simals_instbuf[simals_instptr[i_ptr]] == '#'))
	{
		i = asktool(vhdl_tool, x_("get-line"), (INTBIG)line);

		if (i == 0)
		{
			++i_ptr;
			return(TRUE);
		}

		if (estrlen(line) > 255)
		{
			ttyputerr(_("ERROR: Input record too large.  Make them smaller"));
			return(TRUE);
		}
		if (simals_fragment_line(line)) return(TRUE);
		i_ptr = 0;
	}

	(void)estrcpy(sp, &(simals_instbuf[simals_instptr[i_ptr]]));
	++i_ptr;
	return(FALSE);
}

/*
 * Name: simals_get_int
 *
 * Description:
 *	This procedure reads in the required number of strings to compose an
 * integer value.  It is possible to have a leading +/- sign before the actual
 * integer value.
 *
 * Calling Arguments:
 *	value = pointer to an integer value where results are to be saved
 */
BOOLEAN simals_get_int(INTBIG *value)
{
	BOOLEAN   i;
	CHAR  s1[256], s2[256];

	i = simals_get_string(s1);
	if (i) return(TRUE);

	if ((s1[0] == '+') || (s1[0] == '-'))
	{
		i = simals_get_string(s2);
		if (i) return(TRUE);
		(void)estrcat(s1, s2);
	}

	*value = simals_atoi(s1);
	return(FALSE);
}

/*
 * Name: simals_atoi
 *
 * Description:
 *	This procedure examines the input string, determines which base the
 * integer is written in, and then returns the integer value to the calling
 * routine.  Hex numbers = 0xDDDD, Octal = 0DDDD, Binary = 0bDDDD,
 * and base10 = DDDDD.
 *
 * Calling Arguments:
 *	s1 = pointer to a string containing the integer value to be interpreted
 */
INTBIG simals_atoi(CHAR *s1)
{
	INTBIG  i, sum;

	sum = 0;
	switch (s1[0])
	{
		case '0':
			switch (s1[1])
			{
				case 'B':
					for (i = 2; ; ++i)
					{
						if ((s1[i] >= '0') && (s1[i] <= '1'))
						{
							 sum = (sum << 1) + s1[i] - 48;
							 continue;
						}
						break;
					}
					break;
				case 'X':
					for (i = 2; ; ++i)
					{
						if ((s1[i] >= '0') && (s1[i] <= '9'))
						{
							 sum = (sum << 4) + s1[i] - 48;
							 continue;
						}
						if ((s1[i] >= 'A') && (s1[i] <= 'F'))
						{
							 sum = (sum << 4) + s1[i] - 55;
							 continue;
						}
						break;
					}
					break;
				default:
					for (i = 1; ; ++i)
					{
						if ((s1[i] >= '0') && (s1[i] <= '7'))
						{
							 sum = (sum << 3) + s1[i] - 48;
							 continue;
						}
						break;
					}
			}
			break;

		default:
			sum = eatol(s1);
	}

	return(sum);
}

/*
 * Name: simals_convert_to_upper
 *
 * Description:
 *	This procedure converts any lower case characters that are in the
 * character string to upper case.
 *
 * Calling Arguments:
 *	s1 = pointer to char string that is to be converted to upper case
 */
void simals_convert_to_upper(CHAR *s1)
{
	for (; *s1; ++s1)
	{
		if (('a' <= (*s1)) && ((*s1) <= 'z')) *s1 += 'A' - 'a';
	}
}

/*
 * Name: simals_get_float
 *
 * Description:
 *	This procedure reads in the required number of strings to compose a
 * float value.  It is possible to have a leading +/- sign before the actual
 * float value combined with the chance that the number is entered in scientific
 * notation.
 *
 * Calling Arguments:
 *	value = pointer to a float value where results are to be saved
 */
BOOLEAN simals_get_float(float *value)
{
	BOOLEAN   eof;
	INTBIG    i;
	CHAR  s1[256], s2[256];

	eof = simals_get_string(s1);
	if (eof) return(TRUE);

	if ((s1[0] == '+') || (s1[0] == '-'))
	{
		eof = simals_get_string(s2);
		if (eof) return(TRUE);
		(void)estrcat(s1, s2);
	}

	i = estrlen(s1);
	if (s1[i-1] != 'E')
	{
		*value = (float)eatof(s1);
		return(FALSE);
	}

	eof = simals_get_string(s2);
	if (eof) return(TRUE);
	(void)estrcat(s1, s2);

	if ((s2[0] == '+') || (s2[0] == '-'))
	{
		eof = simals_get_string(s2);
		if (eof) return(TRUE);
		(void)estrcat(s1, s2);
	}

	*value = (float)eatof(s1);
	return(FALSE);
}

/*
 * Name: simals_get_name
 *
 * Description:
 *	This procedure reads in the required number of strings to compose a
 * model/node name for the element. If array subscripting is used, the
 * brackets and argument string is spliced to the node name.
 *
 * Calling Arguments:
 *	sp = pointer to a character string where results are to be saved
 */
BOOLEAN simals_get_name(CHAR *sp)
{
	BOOLEAN   eof;
	CHAR  s1[256], s2[256];

	eof = simals_get_string(s1);
	if (eof) return(TRUE);

	eof = simals_get_string(s2);
	if (eof || (s2[0] != '['))
	{
		(void)estrcpy(sp, s1);
		--i_ptr;
		return(FALSE);
	}

	(void)estrcat(s1, s2);
	for(;;)
	{
		eof = simals_get_string(s2);
		if (eof) return(TRUE);
		(void)estrcat(s1, s2);
		if (s2[0] == ']') break;
	}

	(void)estrcpy(sp, s1);
	return(FALSE);
}

/*
 * Name: simals_fragment_line
 *
 * Description:
 *	This procedure processes the string specified by the calling argument
 * and fragments it into a series of smaller character strings, each of which
 * is terminated by a null character.  Returns true on error.
 *
 * Calling Arguments:
 *	line = pointer to the character string to be fragmented
 */
BOOLEAN simals_fragment_line(CHAR *line)
{
	INTBIG   i, j, k, count;

	j = count = simals_instptr[0] = 0;
	k = 1;

	for (i = 0; ; ++i)
	{
		if (j > (simals_ibufsize - 3))
		{
			if (simals_alloc_char_buffer(&simals_instbuf, &simals_ibufsize,
				(simals_ibufsize * 5))) return(TRUE);
		}
		if (k > (simals_iptrsize - 2))
		{
			if (simals_alloc_ptr_buffer(&simals_instptr, &simals_iptrsize,
				(simals_iptrsize * 5))) return(TRUE);
		}

		if (line[i] == 0 || line[i] == '\n')
		{
			if (count)
			{
				simals_instbuf[j] = 0;
				simals_instptr[k] = -1;
			} else
			{
				simals_instptr[k-1] = -1;
			}
			break;
		}

		switch (line[i])
		{
			case ' ':
			case ',':
			case '\t':
			case ':':
				if (count)
				{
					simals_instbuf[j] = 0;
					simals_instptr[k] = j+1;
					++j;
					++k;
					count = 0;
				}
				break;

			case '(':
			case ')':
			case '{':
			case '}':
			case '[':
			case ']':
			case '=':
			case '!':
			case '>':
			case '<':
			case '+':
			case '-':
			case '*':
			case '/':
			case '%':
			case '@':
			case ';':
			case '#':
				if (count)
				{
					simals_instbuf[j] = 0;
					simals_instbuf[j+1] = line[i];
					simals_instbuf[j+2] = 0;
					simals_instptr[k] = j+1;
					simals_instptr[k+1] = j+3;
					j += 3;
					k += 2;
					count = 0;
				} else
				{
					simals_instbuf[j] = line[i];
					simals_instbuf[j+1] = 0;
					simals_instptr[k] = j+2;
					j += 2;
					++k;
				}
				break;

			default:
				if (('a' <= line[i]) && (line[i] <= 'z'))
				{
					line[i] += 'A' - 'a';
				}
				simals_instbuf[j] = line[i];
				++j;
				++count;
		}
	}
	return(FALSE);
}

/*
 * Name: simals_read_net_desc
 *
 * Description:
 *	This procedure reads a netlist description of the logic network
 * to be analysed in other procedures.  Returns true on error.
 */
BOOLEAN simals_read_net_desc(NODEPROTO *np)
{
	CHAR    s1[256], *intended;
	BOOLEAN    eof;
	extern TOOL *vhdl_tool;

	simals_mainproto = np;
	if (asktool(vhdl_tool, x_("begin-netlist-input"), (INTBIG)np, sim_filetypeals, (INTBIG)&intended) != 0)
	{
		ttyputerr(_("Cannot read %s"), intended);
		return(TRUE);
	}
	ttyputmsg(_("Simulating netlist in %s"), intended);

	simals_instptr[0] = -1;
	i_ptr = 0;
	simals_loop_count = 0;

	for(;;)
	{
		eof = simals_get_string(s1);
		if (eof) break;

		if (! (estrcmp(s1, x_("GATE")) && estrcmp(s1, x_("FUNCTION")) && estrcmp(s1, x_("MODEL"))))
		{
			if (simals_parse_struct_header(s1[0]))
			{
				(void)asktool(vhdl_tool, x_("end-input"));
				return(TRUE);
			}
			continue;
		}

		ttyputerr(_("ERROR: String '%s' invalid (expecting gate, function, or model)"), s1);
		(void)asktool(vhdl_tool, x_("end-input"));
		return(TRUE);
	}
	(void)asktool(vhdl_tool, x_("end-input"));
	return(FALSE);
}

/*
 * Name: simals_parse_struct_header
 *
 * Description:
 *	This procedure parses the input text used to describe the header for
 * a top level structure (gate, function, model).  The structure name and
 * argument list (exported node names) are entered into the database.  Returns
 * nonzero on error.
 *
 * Calling Arguments:
 *	flag = char representing the type of structure to be parsed
 */
BOOLEAN simals_parse_struct_header(CHAR flag)
{
	BOOLEAN   eof;
	CHAR  s1[256];
	MODEL **modptr1;
	EXPORT **exptr1;

	eof = simals_get_name(s1);
	if (eof)
	{
		ttyputerr(_("Structure declaration: EOF unexpectedly found"));
		return(TRUE);
	}

	modptr1 = &simals_modroot;
	for(;;)
	{
		if (*modptr1 == 0)
		{
			simals_modptr2 = (MODPTR) simals_alloc_mem((INTBIG)sizeof(MODEL));
			if (simals_modptr2 == 0) return(TRUE);
			simals_modptr2->name = simals_alloc_mem((INTBIG)(estrlen(s1) + 1));
			if (simals_modptr2->name == 0) return(TRUE);
			(void)estrcpy(simals_modptr2->name, s1);
			simals_modptr2->type = flag;
			simals_modptr2->ptr = 0;
			simals_modptr2->exptr  = 0;
			simals_modptr2->setptr = 0;
			simals_modptr2->loadptr = 0;
			simals_modptr2->fanout = 1;
			simals_modptr2->priority = 1;
			simals_modptr2->next   = 0;
			*modptr1 = simals_modptr2;
			break;
		}
		simals_modptr2 = *modptr1;
		if (! estrcmp(simals_modptr2->name, s1))
		{
			ttyputerr(_("ERROR: Structure %s already defined"), s1);
			return(TRUE);
		}
		modptr1 = &simals_modptr2->next;
	}

	eof = simals_get_string(s1);
	if (eof)
	{
		ttyputerr(_("Structure declaration: EOF unexpectedly found"));
		return(TRUE);
	}
	if (s1[0] != '(')
	{
		ttyputerr(_("Structure declaration: Expecting to find '(' in place of string '%s'"), s1);
		return(TRUE);
	}

	for(;;)
	{
		eof = simals_get_name(s1);
		if (eof)
		{
			ttyputerr(_("Structure declaration: EOF unexpectedly found"));
			return(TRUE);
		}
		if (s1[0] == ')') break;

		exptr1 = &simals_modptr2->exptr;
		for(;;)
		{
			if (*exptr1 == 0)
			{
				simals_exptr2 = (EXPTR) simals_alloc_mem((INTBIG)sizeof(EXPORT));
				if (simals_exptr2 == 0) return(TRUE);
				simals_exptr2->node_name = simals_alloc_mem((INTBIG)(estrlen(s1) + 1));
				if (simals_exptr2->node_name == 0) return(TRUE);
				(void)estrcpy(simals_exptr2->node_name, s1);
				simals_exptr2->nodeptr = 0;
				simals_exptr2->next = 0;
				*exptr1 = simals_exptr2;
				break;
			}
			simals_exptr2 = *exptr1;
			if (! estrcmp(simals_exptr2->node_name, s1))
			{
				ttyputerr(_("Node %s specified more than once in argument list"), s1);
				return(TRUE);
			}
			exptr1 = &simals_exptr2->next;
		}
	}

	switch (flag)
	{
		case 'G':
			if (simals_parse_gate()) return(TRUE);
			return(FALSE);
		case 'F':
			if (simals_parse_function()) return(TRUE);
			return(FALSE);
		case 'M':
			if (simals_parse_model()) return(TRUE);
			return(FALSE);
	}
	ttyputerr(_("Error in parser: invalid structure type"));
	return(TRUE);
}

/*
 * Name: simals_parse_gate
 *
 * Description:
 *	This procedure parses the text used to describe a gate entity.
 * The user specifies truth table entries, loading factors, and timing parameters
 * in this region of the netlist.  Returns true on error.
 */
BOOLEAN simals_parse_gate(void)
{
	INTBIG   j;
	BOOLEAN eof;
	CHAR  s1[256];

	/* init delay transition name */
	(void)esnprintf(delay, 16, x_("XX"));

	simals_delta_def = simals_linear_def = simals_exp_def = simals_random_def = simals_abs_def = 0;
	simals_rowptr1 = &(simals_modptr2->ptr);
	for(;;)
	{
		eof = simals_get_string(s1);
		if (eof || (! (estrcmp(s1, x_("GATE")) && estrcmp(s1, x_("FUNCTION")) && estrcmp(s1, x_("MODEL")))))
		{
			--i_ptr;
			break;
		}

		if (! estrcmp(s1, x_("I")))
		{
			simals_rowptr2 = (ROWPTR) simals_alloc_mem((INTBIG)sizeof(ROW));
			if (simals_rowptr2 == 0) return(TRUE);
			simals_rowptr2->inptr = 0;
			simals_rowptr2->outptr = 0;
			simals_rowptr2->delta = simals_delta_def;
			simals_rowptr2->linear = simals_linear_def;
			simals_rowptr2->exp = simals_exp_def;
			simals_rowptr2->random = simals_random_def;
			simals_rowptr2->abs = simals_abs_def;
			(void)allocstring(&(simals_rowptr2->delay), delay, sim_tool->cluster);
			(void)esnprintf(delay, 16, x_("XX"));  /* reset name */
			simals_rowptr2->next = 0;
			*simals_rowptr1 = (CHAR*) simals_rowptr2;
			simals_rowptr1 = (CHAR**) &(simals_rowptr2->next);
			simals_ioptr1 = (CHAR**) &(simals_rowptr2->inptr);
			if (simals_parse_node()) return(TRUE);
			continue;
		}

		if (! estrcmp(s1, x_("O")))
		{
			simals_ioptr1 = (CHAR**) &(simals_rowptr2->outptr);
			if (simals_parse_node()) return(TRUE);
			continue;
		}

		if (! estrcmp(s1, x_("T")))
		{
			if (simals_parse_timing()) return(TRUE);
			continue;
		}

		if (! estrcmp(s1, x_("D")))
		{
			if (simals_parse_delay()) return(TRUE);
			continue;
		}

		if (! estrcmp(s1, x_("FANOUT")))
		{
			if (simals_parse_fanout()) return(TRUE);
			continue;
		}

		if (! estrcmp(s1, x_("LOAD")))
		{
			if (simals_parse_load()) return(TRUE);
			continue;
		}

		if (! estrcmp(s1, x_("PRIORITY")))
		{
			eof = simals_get_int(&j);
			if (eof)
			{
				ttyputerr(_("Priority declaration: EOF unexpectedly found"));
				return(TRUE);
			}
			simals_modptr2->priority = (INTSML)j;
			continue;
		}

		if (! estrcmp(s1, x_("SET")))
		{
			simals_ioptr1 = (CHAR**) &(simals_modptr2->setptr);
			for (simals_ioptr2 = simals_modptr2->setptr; simals_ioptr2;
				simals_ioptr2 = simals_ioptr2->next)
			{
				simals_ioptr1 = (CHAR**) &(simals_ioptr2->next);
			}
			if (simals_parse_node()) return(TRUE);
			continue;
		}

		ttyputerr(_("ERROR: String '%s' invalid syntax"), s1);
		return(TRUE);
	}
	return(FALSE);
}

/*
 * Name: simals_parse_node
 *
 * Description:
 *	This procedure creates an entry in the database for one of the nodes
 * that belong to a row entry or set state entry.  Returns true on error.
 */
BOOLEAN simals_parse_node(void)
{
	INTBIG   j;
	BOOLEAN  eof;
	CHAR  s1[256];

	for(;;)
	{
		eof = simals_get_name(s1);
		if (eof || (! (estrcmp(s1, x_("GATE")) && estrcmp(s1, x_("FUNCTION"))
			&& estrcmp(s1, x_("MODEL")) && estrcmp(s1, x_("I")) && estrcmp(s1, x_("O"))
			&& estrcmp(s1, x_("T")) && estrcmp(s1, x_("FANOUT")) && estrcmp(s1, x_("LOAD"))
			&& estrcmp(s1, x_("PRIORITY")) && estrcmp(s1, x_("SET")))))
		{
			--i_ptr;
			break;
		}

		simals_ioptr2 = (IOPTR) simals_alloc_mem((INTBIG)sizeof(IO));
		if (simals_ioptr2 == 0) return(TRUE);
		simals_ioptr2->nodeptr = (NODE *)simals_alloc_mem((INTBIG)(estrlen(s1) + 1));
		if (simals_ioptr2->nodeptr == 0) return(TRUE);
		(void)estrcpy((CHAR *)simals_ioptr2->nodeptr, s1);
		simals_ioptr2->strength = GATE_STRENGTH;
		simals_ioptr2->next = 0;
		*simals_ioptr1 = (CHAR*) simals_ioptr2;
		simals_ioptr1 = (CHAR**) &(simals_ioptr2->next);

		eof = simals_get_string(s1);
		if (eof)
		{
			ttyputerr(_("Node declaration: EOF unexpectedly found"));
			return(TRUE);
		}
		switch (s1[0])
		{
			case '=':
			case '!':
			case '>':
			case '<':
			case '+':
			case '-':
			case '*':
			case '/':
			case '%':
				break;
			default:
				ttyputerr(_("Gate declaration: Invalid Operator '%s'"), s1);
				return(TRUE);
		}
		simals_ioptr2->operatr = s1[0];

		eof = simals_get_string(s1);
		if (eof)
		{
			ttyputerr(_("Node declaration: EOF unexpectedly found"));
			return(TRUE);
		}
		if (! (estrcmp(s1, x_("L")) && estrcmp(s1, x_("X")) && estrcmp(s1, x_("H"))))
		{
			simals_ioptr2->operand = (CHAR*) simals_trans_state_to_number(s1);
		} else
		{
			--i_ptr;
			if ((s1[0] == '+') || (s1[0] == '-') || ((s1[0] >= '0') && (s1[0] <= '9')))
			{
				eof = simals_get_int((INTBIG *)&simals_ioptr2->operand);
				if (eof)
				{
					ttyputerr(_("Node declaration: EOF unexpectedly found"));
					return(TRUE);
				}
			} else
			{
				eof = simals_get_name(s1);
				if (eof)
				{
					ttyputerr(_("Node declaration: EOF unexpectedly found"));
					return(TRUE);
				}
				simals_ioptr2->operand = simals_alloc_mem((INTBIG)(estrlen(s1) + 1));
				if (simals_ioptr2->operand == 0) return(TRUE);
				(void)estrcpy(simals_ioptr2->operand, s1);
				simals_ioptr2->operatr += 128;
			}
		}

		eof = simals_get_string(s1);
		if (eof || (s1[0] != '@'))
		{
			--i_ptr;
			continue;
		}
		eof = simals_get_int(&(j));
		if (eof)
		{
			ttyputerr(_("Node declaration: EOF unexpectedly found"));
			return(TRUE);
		}
		simals_ioptr2->strength = (INTSML)j*2;
	}
	return(FALSE);
}

/*
 * Name: simals_trans_state_to_number
 *
 * Description:
 *	This procedure translates a state representation (L, H, X) that is
 * stored in a char to an integer value.
 *
 * Calling Arguments:
 *	s1 = pointer to character string that contains state value
 */
INTBIG simals_trans_state_to_number(CHAR *s1)
{
	switch (s1[0])
	{
		case 'L': case 'l':
			return(LOGIC_LOW);
		case 'X': case 'x':
			return(LOGIC_X);
		case 'H': case 'h':
			return(LOGIC_HIGH);
		default:
			return(simals_atoi(s1));
	}
}

/*
 * Name: simals_parse_timing
 *
 * Description:
 *	This procedure inserts timing values into the appropriate places in
 * the database.  Returns true on error.
 */
BOOLEAN simals_parse_timing(void)
{
	BOOLEAN    eof;
	CHAR   s1[256], s2[256];
	float  value;

	simals_delta_def = simals_linear_def = simals_exp_def = simals_random_def = simals_abs_def = 0;

	for(;;)
	{
		eof = simals_get_string(s1);
		if (eof)
		{
			ttyputerr(_("Timing declaration: EOF unexpectedly found"));
			return(TRUE);
		}

		eof = simals_get_string(s2);
		if (eof)
		{
			ttyputerr(_("Timing declaration: EOF unexpectedly found"));
			return(TRUE);
		}
		if (s2[0] != '=')
		{
			ttyputerr(_("Timing declaration: Invalid Operator '%s' (expecting '=')"), s2);
			return(TRUE);
		}

		eof = simals_get_float(&value);
		if (eof)
		{
			ttyputerr(_("Timing declaration: EOF unexpectedly found"));
			return(TRUE);
		}

		switch (s1[0])
		{
			case 'A':
				simals_abs_def = value;
				break;
			case 'D':
				simals_delta_def = value;
				break;
			case 'E':
				simals_exp_def = value;
				break;
			case 'L':
				simals_linear_def = value;
				break;
			case 'R':
				simals_random_def = value;
				if (value > 0.0) simals_modptr2->priority = 2;
				break;
			default:
				ttyputerr(_("Invalid timing mode '%s'"), s1);
				return(TRUE);
		}

		eof = simals_get_string(s1);
		if (eof || (s1[0] != '+'))
		{
			--i_ptr;
			break;
		}
	}
	return(FALSE);
}

/*
 * Name: simals_parse_delay
 *
 * This procedure sets the delay transition type for the current input state.
 */
BOOLEAN simals_parse_delay(void)
{
	BOOLEAN   eof;
	CHAR  s1[256];

	eof = simals_get_string(s1);
	if (eof)
	{
		ttyputerr(_("Timing declaration: EOF unexpectedly found"));
		return(TRUE);
	}

	if (estrcmp(s1, x_("01")) && estrcmp(s1, x_("10")) && estrcmp(s1, x_("OZ")) && estrcmp(s1, x_("Z1"))
		&& estrcmp(s1, x_("1Z")) && estrcmp(s1, x_("Z0")) && estrcmp(s1, x_("0X")) && estrcmp(s1, x_("X1"))
		&& estrcmp(s1, x_("1X")) && estrcmp(s1, x_("X0")) && estrcmp(s1, x_("XZ")) && estrcmp(s1, x_("ZX")))
	{
		ttyputerr(_("Invalid delay transition name '%s'"), s1);
		return(TRUE);
	}
	else
		(void)esnprintf(delay, 16, x_("%s"), s1);

	return(FALSE);
}

/*
 * Name: simals_parse_fanout
 *
 * Description:
 *	This procedure sets a flag in the model data structure regarding
 * if fanout calculations are to be performed for this models output.
 * If fanout calculations are required the model should have a single output.
 * Returns true on error.
 */
BOOLEAN simals_parse_fanout(void)
{
	BOOLEAN   eof;
	CHAR  s1[256];

	eof = simals_get_string(s1);
	if (eof)
	{
		ttyputerr(_("Fanout declaration: EOF unexpectedly found"));
		return(TRUE);
	}
	if (s1[0] != '=')
	{
		ttyputerr(_("Fanout declaration: Invalid Operator '%s' (expecting '=')"), s1);
		return(TRUE);
	}

	eof = simals_get_string(s1);
	if (eof)
	{
		ttyputerr(_("Fanout declaration: EOF unexpectedly found"));
		return(TRUE);
	}

	if (estrcmp(s1, x_("ON")) == 0)
	{
		simals_modptr2->fanout = 1;
		return(FALSE);
	}

	if (estrcmp(s1, x_("OFF")) == 0)
	{
		simals_modptr2->fanout = 0;
		return(FALSE);
	}

	ttyputerr(_("Fanout declaration: Invalid option '%s'"), s1);
	return(TRUE);
}

/*
 * Name: simals_parse_load
 *
 * Description:
 *	This procedure enters the capacitive load rating (on per unit basis)
 * into the database for the specified node.  Returns true on error.
 */
BOOLEAN simals_parse_load(void)
{
	BOOLEAN	eof;
	CHAR    s1[256], s2[256];
	float    load;
	LOADPTR  loadptr2;
	LOAD   **loadptr1;

	loadptr1 = &simals_modptr2->loadptr;
	for (loadptr2 = simals_modptr2->loadptr; loadptr2 != 0; loadptr2 = loadptr2->next)
		loadptr1 = &loadptr2->next;

	for(;;)
	{
		eof = simals_get_name(s1);
		if (eof || (! (estrcmp(s1, x_("GATE")) && estrcmp(s1, x_("FUNCTION"))
			&& estrcmp(s1, x_("MODEL")) && estrcmp(s1, x_("I")) && estrcmp(s1, x_("O"))
			&& estrcmp(s1, x_("T")) && estrcmp(s1, x_("FANOUT")) && estrcmp(s1, x_("LOAD"))
			&& estrcmp(s1, x_("PRIORITY")) && estrcmp(s1, x_("SET")))))
		{
			--i_ptr;
			break;
		}

		eof = simals_get_string(s2);
		if (eof)
		{
			ttyputerr(_("Load declaration: EOF unexpectedly found"));
			return(TRUE);
		}
		if (s2[0] != '=')
		{
			ttyputerr(_("Load declaration: Invalid Operator '%s' (expecting '=')"), s2);
			return(TRUE);
		}

		eof = simals_get_float(&load);
		if (eof)
		{
			ttyputerr(_("Load declaration: EOF unexpectedly found"));
			return(TRUE);
		}

		loadptr2 = (LOADPTR) simals_alloc_mem((INTBIG)sizeof(LOAD));
		if (loadptr2 == 0) return(TRUE);
		loadptr2->ptr = simals_alloc_mem((INTBIG)(estrlen(s1) + 1));
		if (loadptr2->ptr == 0) return(TRUE);
		(void)estrcpy(loadptr2->ptr, s1);
		loadptr2->load = load;
		loadptr2->next = 0;
		*loadptr1 = loadptr2;
		loadptr1 = &loadptr2->next;
	}
	return(FALSE);
}

/*
 * Name: simals_parse_model
 *
 * Description:
 *	This procedure parses the text used to describe a model entity.
 * The user specifies the interconnection of lower level primitives (gates and
 * functions) in this region of the netlist.  Returns true on error.
 */
BOOLEAN simals_parse_model(void)
{
	BOOLEAN    eof;
	CHAR    s1[256];
	EXPORT **exptr1;
	CONNECT **conptr1;

	for(;;)
	{
		eof = simals_get_name(s1);
		if (eof || (! (estrcmp(s1, x_("GATE")) && estrcmp(s1, x_("FUNCTION")) && estrcmp(s1, x_("MODEL")))))
		{
			--i_ptr;
			break;
		}

		if (s1[0] == '}') continue;

		if (! estrcmp(s1, x_("SET")))
		{
			simals_ioptr1 = (CHAR**) &(simals_modptr2->setptr);
			for (simals_ioptr2 = simals_modptr2->setptr; simals_ioptr2;
				simals_ioptr2 = simals_ioptr2->next)
			{
				simals_ioptr1 = (CHAR**) &(simals_ioptr2->next);
			}
			if (simals_parse_node()) return(TRUE);
			continue;
		}

		conptr1 = (CONNECT **)&simals_modptr2->ptr;
		for(;;)
		{
			if (*conptr1 == 0)
			{
				simals_conptr2 = (CONPTR) simals_alloc_mem((INTBIG)sizeof(CONNECT));
				if (simals_conptr2 == 0) return(TRUE);
				simals_conptr2->inst_name = simals_alloc_mem((INTBIG)(estrlen(s1) + 1));
				if (simals_conptr2->inst_name == 0) return(TRUE);
				(void)estrcpy(simals_conptr2->inst_name, s1);
				simals_conptr2->model_name = 0;
				simals_conptr2->exptr = 0;
				simals_conptr2->next = 0;
				simals_conptr2->display_page = 0;
				*conptr1 = simals_conptr2;
				break;
			}
			simals_conptr2 = *conptr1;
			if (! estrcmp(simals_conptr2->inst_name, s1))
			{
				ttyputerr(_("ERROR: Instance name '%s' defined more than once"), s1);
				return(TRUE);
			}
			conptr1 = &simals_conptr2->next;
		}

		eof = simals_get_name(s1);
		if (eof)
		{
			ttyputerr(_("Model declaration: EOF unexpectedly found"));
			return(TRUE);
		}
		simals_conptr2->model_name = simals_alloc_mem((INTBIG)(estrlen(s1) + 1));
		if (simals_conptr2->model_name == 0) return(TRUE);
		(void)estrcpy(simals_conptr2->model_name, s1);


		eof = simals_get_string(s1);
		if (eof)
		{
			ttyputerr(_("Model declaration: EOF unexpectedly found"));
			return(TRUE);
		}
		if (s1[0] != '(')
		{
			ttyputerr(_("Model declaration: Expecting to find '(' in place of string '%s'"), s1);
			return(TRUE);
		}

		exptr1 = &simals_conptr2->exptr;
		for(;;)
		{
			eof = simals_get_name(s1);
			if (eof)
			{
				ttyputerr(_("Model declaration: EOF unexpectedly found"));
				return(TRUE);
			}
			if (s1[0] == ')') break;
			simals_exptr2 = (EXPTR) simals_alloc_mem((INTBIG)sizeof(EXPORT));
			if (simals_exptr2 == 0) return(TRUE);
			simals_exptr2->nodeptr = 0;
			simals_exptr2->node_name = simals_alloc_mem((INTBIG)(estrlen(s1) + 1));
			if (simals_exptr2->node_name == 0) return(TRUE);
			(void)estrcpy(simals_exptr2->node_name, s1);
			simals_exptr2->next = 0;
			*exptr1 = simals_exptr2;
			exptr1 = &simals_exptr2->next;
		}
	}
	return(FALSE);
}

/*
 * Name: simals_parse_function
 *
 * Description:
 *      This procedure parses the text used to describe a function entity.
 * The user specifies input entries, loading factors, and timing parameters
 * in this region of the netlist.
 */
BOOLEAN simals_parse_function(void)
{
	BOOLEAN    eof;
	CHAR     s1[256];
	INTBIG    j;
	FUNCPTR  funchead;
	EXPORT **exptr1;

	simals_modptr2->fanout = 0;
	simals_modptr2->ptr = simals_alloc_mem(sizeof(FUNC));
	if (simals_modptr2->ptr == 0) return(TRUE);
	funchead = (FUNCPTR)simals_modptr2->ptr;
	funchead->procptr = 0;
	funchead->inptr = 0;
	funchead->delta = 0.0;
	funchead->linear = 0.0;
	funchead->exp = 0.0;
	funchead->abs = 0.0;
	funchead->random = 0.0;
	funchead->userptr = 0;
	funchead->userint = 0;
	funchead->userfloat = 0.0;

	for(;;)
	{
		eof = simals_get_string(s1);
		if (eof || (!(estrcmp(s1, x_("GATE")) && estrcmp(s1, x_("FUNCTION")) && estrcmp(s1, x_("MODEL")))))
		{
			--i_ptr;
			break;
		}

		if (!estrcmp(s1, x_("I")))
		{
			exptr1 = &funchead->inptr;
			for (simals_exptr2 = funchead->inptr; simals_exptr2;
				simals_exptr2 = simals_exptr2->next)
			{
				exptr1 = &simals_exptr2->next;
			}
			if (simals_parse_func_input(exptr1)) return(TRUE);
			continue;
		}

		if (!estrcmp(s1, x_("O")))
		{
			if (simals_parse_func_output()) return(TRUE);
			continue;
		}

		if (!estrcmp(s1, x_("T")))
		{
			if (simals_parse_timing()) return(TRUE);
			funchead->delta = simals_delta_def;
			funchead->linear = simals_linear_def;
			funchead->exp = simals_exp_def;
			funchead->abs = simals_abs_def;
			funchead->random = simals_random_def;
			continue;
		}

		if (!estrcmp(s1, x_("LOAD")))
		{
			if (simals_parse_load()) return(TRUE);
			continue;
		}

		if (!estrcmp(s1, x_("PRIORITY")))
		{
			eof = simals_get_int(&j);
			if (eof)
			{
				ttyputerr(_("Priority declaration: EOF unexpectedly found"));
				return(TRUE);
			}
			simals_modptr2->priority = (INTSML)j;
			continue;
		}

		if (!estrcmp(s1, x_("SET")))
		{
			simals_ioptr1 = (CHAR**) &(simals_modptr2->setptr);
			for (simals_ioptr2 = simals_modptr2->setptr; simals_ioptr2;
				simals_ioptr2 = simals_ioptr2->next)
			{
				simals_ioptr1 = (CHAR**) &(simals_ioptr2->next);
			}
			(void)simals_parse_node();
			continue;
		}

		ttyputerr(_("ERROR: String '%s' invalid syntax"), s1);
		return(TRUE);
	}
	return(FALSE);
}

/*
 * Name: simals_parse_func_input
 *
 * Description:
 *      This procedure creates a list of input nodes which are used for event
 * driving the function.
 */
BOOLEAN simals_parse_func_input(EXPORT **exptr1)
{
	BOOLEAN   eof;
	CHAR    s1[256];

	for(;;)
	{
		eof = simals_get_name(s1);
		if (eof || (!(estrcmp(s1, x_("GATE")) && estrcmp(s1, x_("FUNCTION"))
			&& estrcmp(s1, x_("MODEL")) && estrcmp(s1, x_("I")) && estrcmp(s1, x_("O"))
			&& estrcmp(s1, x_("T")) && estrcmp(s1, x_("FANOUT")) && estrcmp(s1, x_("LOAD"))
			&& estrcmp(s1, x_("PRIORITY")) && estrcmp(s1, x_("SET")))))
		{
			--i_ptr;
			break;
		}

		simals_exptr2 = (EXPTR)simals_alloc_mem(sizeof(EXPORT));
		if (simals_exptr2 == 0) return(TRUE);
		simals_exptr2->nodeptr = 0;
		simals_exptr2->node_name = simals_alloc_mem(estrlen(s1) + 1);
		if (simals_exptr2->node_name == 0) return(TRUE);
		(void)estrcpy(simals_exptr2->node_name, s1);
		simals_exptr2->next = 0;
		*exptr1 = simals_exptr2;
		exptr1 = &simals_exptr2->next;
	}
	return(FALSE);
}

/*
 * Name: simals_parse_func_output
 *
 * Description:
 *      This procedure creates a list of output nodes for the function.
 */
BOOLEAN simals_parse_func_output(void)
{
	BOOLEAN   eof;
	CHAR    s1[256];

	for(;;)
	{
		eof = simals_get_name(s1);
		if (eof || (!(estrcmp(s1, x_("GATE")) && estrcmp(s1, x_("FUNCTION"))
			&& estrcmp(s1, x_("MODEL")) && estrcmp(s1, x_("I")) && estrcmp(s1, x_("O"))
			&& estrcmp(s1, x_("T")) && estrcmp(s1, x_("FANOUT")) && estrcmp(s1, x_("LOAD"))
			&& estrcmp(s1, x_("PRIORITY")) && estrcmp(s1, x_("SET")))))
		{
			--i_ptr;
			break;
		}

		for (simals_exptr2 = simals_modptr2->exptr; ; simals_exptr2 = simals_exptr2->next)
		{
			if (!simals_exptr2)
			{
				ttyputerr(_("ERROR: Unable to find node %s in port list"), s1);
				return(TRUE);
			}
			if (!estrcmp(s1, simals_exptr2->node_name))
			{
				simals_exptr2->nodeptr = (NODEPTR)1;
				break;
			}
		}
	}
	return(FALSE);
}

/*
 * Name: simals_fragment_command
 *
 * Description:
 *	This procedure processes the string specified by the calling argument
 * and fragments it into a series of smaller character strings, each of which
 * is terminated by a null character.  Returns true on error.
 *
 * Calling Arguments:
 *	line = pointer to the character string to be fragmented
 */
BOOLEAN simals_fragment_command(CHAR *line)
{
	INTBIG   i, j, k, count;

	j = count = simals_instptr[0] = simals_instbuf[0] = 0;
	k = 1;

	for (i = 0; ; ++i)
	{
		if (j > (simals_ibufsize - 3))
		{
			if (simals_alloc_char_buffer(&simals_instbuf, &simals_ibufsize,
				(simals_ibufsize * 5))) return(TRUE);
		}
		if (k > (simals_iptrsize - 2))
		{
			if (simals_alloc_ptr_buffer(&simals_instptr, &simals_iptrsize,
				(simals_iptrsize * 5))) return(TRUE);
		}

		if (line[i] == 0 || line[i] == '\n')
		{
			if (count)
			{
				simals_instbuf[j] = 0;
				simals_instptr[k] = -1;
			} else
			{
				simals_instptr[k-1] = -1;
			}
			break;
		}
		switch (line[i])
		{
			case ' ':
			case ',':
			case '\t':
			case '=':
			case '@':
				if (count)
				{
					simals_instbuf[j] = 0;
					simals_instptr[k] = j+1;
					++j;
					++k;
					count = 0;
				}
				break;

			default:
				simals_instbuf[j] = line[i];
				++j;
				++count;
		}
	}
	return(FALSE);
}

/*
 * Name: simals_compute_node_name
 *
 * Description:
 *	This procedure composes a character string which indicates the node name
 * for the nodeptr specified in the calling argument.
 *
 * Calling Arguments:
 *	nodehead = pointer to desired node in database
 *	sp	= pointer to char string where complete name is to be saved
 */
void simals_compute_node_name(NODEPTR nodehead, CHAR *sp)
{
	CONPTR  cellhead;
	EXPTR   exhead;

	cellhead = nodehead->cellptr;
	estrcpy(sp, simals_compute_path_name(cellhead));

	for (exhead = cellhead->exptr; ; exhead = exhead->next)
	{
		if (nodehead == exhead->nodeptr)
		{
			(void)estrcat(sp, x_("."));
			(void)estrcat(sp, exhead->node_name);
			return;
		}
	}
}

/*
 * Name: simals_compute_path_name
 *
 * Description:
 *	This procedure composes a character string which indicates the path name
 * to the level of hierarchy specified in the calling argument.
 *
 * Calling Arguments:
 *	cellhead = pointer to desired level of hierarchy
 *	sp	= pointer to char string where path name is to be saved
 */
CHAR *simals_compute_path_name(CONPTR cellhead)
{
	REGISTER void *infstr;

	infstr = initinfstr();
	for ( ; cellhead; cellhead = cellhead->parent)
		formatinfstr(infstr, x_(".%s"), cellhead->inst_name);
	return(returninfstr(infstr));
}

/*
 * Name: simals_find_node
 *
 * Description:
 *	This procedure returns a pointer to the calling routine which indicates
 * the address of the node entry in the database.  The calling argument string
 * contains information detailing the path name to the desired node.
 *
 * Calling Argument:
 *	sp = pointer to char string containing path name to node
 */
NODEPTR simals_find_node(CHAR *sp)
{
	INTBIG	i;
	CHAR    s1[80], *s2;
	CONPTR   cellptr;
	EXPTR    exhead;
	NODEPTR  nodehead;

	if ((sp[0] == '$') && (sp[1] == 'N'))
	{
		i = eatoi(&(sp[2]));
		for (nodehead = simals_noderoot; nodehead; nodehead = nodehead->next)
		{
			if (nodehead->num == i) return(nodehead);
		}
		return(0);
	}

	(void)estrcpy(s1, sp);
	s2 = s1;
	for (i = 0; s1[i]; ++i)
	{
		if (s1[i] == '.') s2 = &(s1[i+1]);
	}

	if (s2 > s1)
	{
		*(s2 - 1) = 0;
		cellptr = simals_find_level(s1);
	} else
	{
		cellptr = simals_levelptr;
	}
	if (cellptr == 0) return(0);

	for (exhead = cellptr->exptr; exhead; exhead = exhead->next)
	{
		if (! estrcmp(exhead->node_name, s2)) return(exhead->nodeptr);
	}

	return(0);
}

/*
 * command parsing routines for instance names
 */
static CONPTR  sim_als_cellptr;

BOOLEAN simals_topofinstances(CHAR **c)
{
	Q_UNUSED( c );
	if (simals_levelptr == 0) return(FALSE);
	sim_als_cellptr = simals_levelptr->child;
	return(TRUE);
}

CHAR *simals_nextinstance(void)
{
	REGISTER void *infstr;

	if (sim_als_cellptr == 0) return(0);
	infstr = initinfstr();
	formatinfstr(infstr, x_("%s (%s)"), sim_als_cellptr->inst_name, sim_als_cellptr->model_name);
	sim_als_cellptr = sim_als_cellptr->next;
	return(returninfstr(infstr));
}

/*
 * Name: simals_find_level
 *
 * Description:
 *	This procedure returns a pointer to a structure in the cross reference
 * table. The calling argument string contains information detailing the path
 * name to the desired level in the cross reference table.
 *
 * Calling Argument:
 *	sp = pointer to char string containing path name to level in xref table
 */
CONPTR simals_find_level(CHAR *sp)
{
	INTBIG    i, j;
	CHAR    s1[80];
	CONPTR  cellptr;

	if (sp[0] == '.')
	{
		cellptr = simals_cellroot;
		if (cellptr == 0) return(0);
		i = 1;
	} else
	{
		if (simals_levelptr == 0) return(0);
		cellptr = simals_levelptr->child;
		i = 0;
	}

	for(j = 0; ; ++i)
	{
		if ((sp[i] == '.') || ! (sp[i]))
		{
			for(s1[j] = 0; ; cellptr = cellptr->next)
			{
				if (! cellptr) return(0);
				if (! estrcmp(s1, cellptr->inst_name))
				{
					if (! (sp[i])) return(cellptr);
					cellptr = cellptr->child;
					j = 0;
					break;
				}
			}
			continue;
		}
		s1[j] = sp[i];
		++j;
	}
}

/*
 * Name: simals_trans_number_to_state
 *
 * Description:
 *	This procedure translates an integer value that represents a state
 * and returns a single character corresponding to the state.
 *
 * Calling Arguments:
 *	state_num = integer value that is to be converted to a character
 */
CHAR *simals_trans_number_to_state(INTBIG state_num)
{
	static CHAR s1[20];

	switch (state_num)
	{
		case LOGIC_LOW:
			(void)estrcpy(s1, x_("L"));
			break;
		case LOGIC_X:
			(void)estrcpy(s1, x_("X"));
			break;
		case LOGIC_HIGH:
			(void)estrcpy(s1, x_("H"));
			break;
		default:
			(void)esnprintf(s1, 20, x_("0x%lX"), state_num);
			break;
	}
	return(s1);
}

#endif  /* SIMTOOL - at top */
