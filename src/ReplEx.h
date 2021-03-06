/*
 * This file is part of the GROMACS molecular simulation package.
 *
 * Copyright (c) 1991-2000, University of Groningen, The Netherlands.
 * Copyright (c) 2001-2004, The GROMACS development team.
 * Copyright (c) 2011,2012,2013,2014,2015,2017, by the GROMACS development team, led by
 * Mark Abraham, David van der Spoel, Berk Hess, and Erik Lindahl,
 * and including many others, as listed in the AUTHORS file in the
 * top-level source directory and at http://www.gromacs.org.
 *
 * GROMACS is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * GROMACS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GROMACS; if not, see
 * http://www.gnu.org/licenses, or write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA.
 *
 * If you want to redistribute modifications to GROMACS, please
 * consider that scientific software is very special. Version
 * control is crucial - bugs must be traceable. We will be happy to
 * consider code for inclusion in the official distribution, but
 * derived work must not be called official GROMACS. Details are found
 * in the README & COPYING files - if they are missing, get the
 * official version at http://www.gromacs.org.
 *
 * To help us fund GROMACS development, we humbly ask that you cite
 * the research papers on the package. Check out http://www.gromacs.org.
 */

#ifndef _repl_ex_h
#define _repl_ex_h

#include <cstdio>

#include "ReplicaState.h"

struct gmx_enerdata_t;
struct gmx_multisim_t;
struct t_commrec;
struct t_inputrec;
class t_state;

/* The parameters for the replica exchange algorithm */
struct ReplicaExchangeParameters
{
    ReplicaExchangeParameters() :
        exchangeInterval(0),
        numExchanges(0),
        randomSeed(-1),
        multisimTitle("Replica_Exchange_Simulation")
    {
    };

    int exchangeInterval; /* Interval in steps at which to attempt exchanges, 0 means no replica exchange */
    int numExchanges;     /* The number of exchanges to attempt at an exchange step */
    int randomSeed;       /* The random seed, -1 means generate a seed */
    string multisimTitle; /* The title of the simulation */
};

/* Abstract type for replica exchange */
typedef struct gmx_repl_ex *gmx_repl_ex_t;

gmx_repl_ex_t
init_replica_exchange(FILE                                    *fplog,
                      float                              temperature,
                      ReplicaExchangeParameters * replExParams);
/* Should only be called on the master ranks */

/*bool replica_exchange(FILE *fplog,
                          const t_commrec *cr,
                          gmx_repl_ex_t re,
                          t_state *state, gmx_enerdata_t *enerd,
                          t_state *state_local,
                          gmx_int64_t step, real time);*/
/* Attempts replica exchange, should be called on all ranks.
 * Returns TRUE if this state has been exchanged.
 * When running each replica in parallel,
 * this routine collects the state on the master rank before exchange.
 * With domain decomposition, the global state after exchange is stored
 * in state and still needs to be redistributed over the ranks.
 */

bool replica_exchange(FILE *fplog, struct gmx_repl_ex *re,
                          ReplicaState *stateGlobal, float enerd, double vol_par,
                          int step, ReplicaExchangeParameters* replExParams);

void print_replica_exchange_statistics(FILE *fplog, gmx_repl_ex_t re);
/* Should only be called on the master ranks */

static void
test_for_replica_exchange(FILE                          *fplog,
                          struct gmx_repl_ex            *re,
                          float                         enerd,
                          double                         vol,
                          int                           step,
                          ReplicaExchangeParameters*    replExParams);

static void print_ind(FILE *fplog, const char *leg, int n, int *ind, int *bEx);
static void print_prob(FILE *fplog, const char *leg, int n, float *prob);
static float calc_delta(FILE *fplog, int bPrint, struct gmx_repl_ex *re, int a, int b, int ap, int bp);
static void
prepare_to_do_exchange(struct gmx_repl_ex *re,
                       const int           replica_id,
                       int                *maxswap,
                       int           *bThisReplicaExchanged);
static void
cyclic_decomposition(const int *destinations,
                     int      **cyclic,
                     int  *incycle,
                     const int  nrepl,
                     int       *nswap);
static void
compute_exchange_order(int     **cyclic,
                       int     **order,
                       const int nrepl,
                       const int maxswap);

static void print_transition_matrix(FILE *fplog, int n, int **nmoves, int *nattempt);
static void print_count(FILE *fplog, const char *leg, int n, int *count);
static void exchange_state(int b, ReplicaState *state);
//static void exchange_float_vecs(int b, float *v, int n);
static void exchange_floats(int b, float *v, int n);
static void exchange_doubles(int b, double *v, int n);
static void exchange_potential(int b, ReplicaState * stateGlobal);
static void exchange_ints(int b, int *v, int n);
static void exchange_cell_list(int b, ReplicaState * stateGlobal);

#endif  /* _repl_ex_h */


