#include "goldberg.h"
#include "maxflow.h"
#include "buffer.h"
struct flow_graph
{
    uint32_t num_nodes;
    uint32_t s;
    uint32_t t;
    uint32_t* capacity;
    uint32_t* flow;
};

uint32_t getCap(struct flow_graph* g, uint32_t from, uint32_t to)
{
    return g->capacity[(from*g->num_nodes) + to];
}

void setCap(struct flow_graph* g, uint32_t from, uint32_t to, uint32_t val)
{
    g->capacity[(from*g->num_nodes) + to]=val;
}

uint32_t getFlow(struct flow_graph* g, uint32_t from,  uint32_t to)
{
    return g->flow[(from*g->num_nodes) + to];
}

void setFlow(struct flow_graph* g, uint32_t from, uint32_t to, uint32_t val)
{
    g->flow[(from*g->num_nodes) + to]=val;
}

static struct node* nodes; 
static struct nighbors* nlist;
static struct Queue active;
static struct flow_graph* in_graph;

void push(struct r_edge* e)
{
    /** condition **/
    if(nodes[e->from].excess==0)
    {
        return;
    }
    if(nodes[e->from].lable != nodes[e->to].lable+1)
    {
        return;
    }
    /** action ***/
    uint32_t to_push;
    if(nodes[e->from].excess > e->residual)
    {
        to_push=e->residual;
    }
    else
    {
        to_push=nodes[e->from].excess;
    }
    nodes[e->from].excess-=to_push;
    nodes[e->to].excess+=to_push;
    e->back_edge->residual+=to_push;
    e->residual=0;
    if(nodes[e->to].excess>0)
    {

        if(e->to != in_graph->t && e->to != in_graph->t)
        {
            active.Enqueue(&active, &(e->to));
        }
    }

}

void relable(uint32_t v)
{
    uint32_t min=0;
    uint32_t i;
    for(i=0; i < nlist[v].size; i++)
    {
        if(nlist[v].list[i].residual>0)
        {
            if(nodes[nlist[v].list[i].to].lable>min)
            {
                min=nodes[nlist[v].list[i].to].lable;
            }
        }
    }
    if(nodes[v].lable<(min+1))
    {
        nodes[v].lable=min+1;
    }
    
}

void sorce_push(struct r_edge* e)
{
    nodes[e->to].excess+=e->residual;
    e->back_edge->residual+=e->residual;
    e->residual=0;
    if(nodes[e->to].excess>0)
    {
        if(e->to!=in_graph->t)
        {
            active.Enqueue(&active, &(e->to));
        }
    }
}

void simpleMaxflow(struct flow_graph* input)
{
    nodes = calloc(input->num_nodes, sizeof(struct node));
    nlist = calloc(input->num_nodes, sizeof(struct nighbors));
    active = new_nonconcurrent_ring_buffer(input->num_nodes*2, sizeof(uint32_t));
    in_graph = input;
    int i;
    for(i=0; i < input->num_nodes; i++)
    {
        nodes[i].id=i;
        nodes[i].lable=0;

        if ( input->s==i )
        {
            nodes[i].lable=input->num_nodes;
        }
        nodes[i].excess=0;
        
        nlist[i].capacity=6;
        nlist[i].size=0;
        nlist[i].list=calloc(6, sizeof(struct r_edge));

    }
    
    for(i=0; i < input->num_nodes; i++)
    {
        /* construct residual graph */
        int j;
        for(j=i; j < input->num_nodes; j++)
        {
            uint32_t cap = getCap(&input, i, j);
            uint32_t cap_ = getCap(&input, j, i);
            if(cap==0 && cap_==0 )
                continue;
            struct r_edge* ji = &(nlist[j].list[nlist[i].size]);
            struct r_edge* ij = &(nlist[i].list[nlist[i].size]);

            nlist[j].list[nlist[i].size].from =j;
            nlist[j].list[nlist[i].size].to =i;
            nlist[j].list[nlist[i].size].residual = cap;
            nlist[j].list[nlist[i].size].back_edge = ij;
            nlist[j].size++;       
            nlist[i].list[nlist[i].size].from =i;
            nlist[i].list[nlist[i].size].to =j;
            nlist[i].list[nlist[i].size].residual = cap_;
            nlist[i].list[nlist[i].size].back_edge = ji;
            nlist[i].size++;
        }
    }
    /* push maximum flow flom sorce */

    for(i=0; i < nlist[input->s].size; i++)
    {
        sorce_push(nlist[input->s].list[i].from);
    }
    /* compute preeflow on residual glaph*/
    for(;;)
    {
        uint32_t* curr_ptr = active.Dequeue(&active);
        if(NULL==curr_ptr)
        {
            break;
        }
        uint32_t curr=*curr_ptr;
        free(curr_ptr);
        int i;
        relable(curr);
        for(i=0; i < nlist[curr].size; i++)
        {
            if(nodes[curr].excess==0)
            {
                break;
            }
            push(&(nlist[curr].list[i]));
            
        }
    }

    /* figure out actual flow */
    
    for(i=0; i < input->num_nodes; i++)
    {
        int j;
        for(j=0; j < nlist[i].size; j++)
        {
            uint32_t from   = nlist[i].list[j].from;
            uint32_t to     = nlist[i].list[j].to;

            uint32_t cap = getCap(input, from, to);
            if(cap > 0)
            {
                setFlow(input, from, to, cap - nodes[i].excess);
            }
            else
            {
                setFlow(input, from, to, 0);
            }
        }
    }

}

BOOL testFlowInvariant(struct flow_graph* input)
{
    /* test not to much flow */
    int i;
    for(i=0; i < input->num_nodes; i++)
    {
        /* construct residual graph */
        int j;
        for(j=0; j < input->num_nodes; j++)
        { 
            if(getCap(input, i, j) < getFlow(input, i, j))
            {
                PRINTF("##### FAILED!!!, Flow bigger then capacity #####");
                return false;
            }
        }
    }

    /* test inflow equals out flow */
    for(i=0; i < input->num_nodes; i++)
    {
        if(input->s==i || input->t==i)
        {
            continue;
        }
        uint32_t inflow=0;
        uint32_t outflow=0;

        int j;
        for(j=0; j < input->num_nodes; j++)
        {
            inflow += getFlow(input, j, i);
            outflow += getFlow(input, j, i);
        }
        if(inflow != outflow)
        {
            PRINTF("########## FAILED!!!! not a flow!!! #########");
            return false;
        }
    }
    PRINTF("********* TEST PASSED!!! ************");
    return true;
}

void testflow()
{
    /* a test graph */
    struct flow_graph testgraph1;
    testgraph1.num_nodes = 6;
    testgraph1.s = 0;
    testgraph1.t = 5;
    testgraph1.capacity = calloc(6*6, sizeof(uint32_t));
    testgraph1.flow     = calloc(6*6, sizeof(uint32_t));
    
    setCap(&testgraph1, 0, 1, 5);
    setCap(&testgraph1, 0, 2, 5);
    setCap(&testgraph1, 1, 4, 3);
    setCap(&testgraph1, 1, 3, 6);
    setCap(&testgraph1, 2, 3, 3);
    setCap(&testgraph1, 2, 4, 1);
    setCap(&testgraph1, 4, 5, 6);
    setCap(&testgraph1, 3, 5, 6);

    simpleMaxflow(&testgraph1);
    testFlowInvariant(&testgraph1);

}

