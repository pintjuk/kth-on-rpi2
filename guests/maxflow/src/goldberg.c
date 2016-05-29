#include "goldberg.h"
#include "maxflow.h"
struct flow_graph()
{
    uint32_t num_nodes;
    uint32_t s;
    uint32_t t;
    uint32_t* capacity;
    uint32_t* flow;
}

uint32_t getCap(flow_graph* g, uint32_t from,  to)
{
    return g->capacity[(from*g.num_nodes) + to];
}
void setCap(flow_graph* g, uint32_t from,  to, uint32_t val)
{
    g->capacity[(from*g.num_nodes) + to]=val;
}

uint32_t getFlow(flow_graph* g, uint32_t from,  to)
{
    return g->flow[(from*g.num_nodes) + to];
}

void setFlow(flow_graph* g, uint32_t from,  to, uint32_t val)
{
    g->flow[(from*g.num_nodes) + to]=val;
}

static node* nodes; 
static nighbors* nlist;
static Queue active;
static flow_graph* in_graph;

void push(r_edge* e)
{
    /** condition **/
    if(nodes[e->from].excess==0)
        return
    if(nodes[e->from].lable != nodes[e->to].lable+1)
        return
    /** action ***/
    uint32_t to_push;
    if(nodes[r_edge->from].excess > r_edge.residual)
    {
        to_push=r_edge.residual;
    else
    {
        to_push=nodes[r_edge->from].excess;
    }
    nodes[r_from->from].excess-=to_push;
    nodes[r_edge->to].excess+=to_pushl;
    e->back_edge->residual+=to_push;
    e->residual=0;
    if(nodes[r_edge->to].excess>0)
    {

        if(r_edge->to!=in_graph->t && r_edge->to != in_graph->t)
            active.Enqueue(&active, &(r_edge->to));
    }

}

void relable(uint32_t v)
{
    uint32_t min=0;
    uint32_t i;
    for(i=0; i < nighbors[v].size; i++)
    {
        if(nighbors[v].list[i].residual>0)
        {
            if(nodes[nighbors[v].list[i].to].lable>min)
            {
                min=nodes[nighbors[v].list[i].to].lable;
            }
        }
    }
    if(nighbors[v].lable<(min+1))
    {
        nighbors[v].lable=min+1;
    }
    
}

void sorce_push(r_edge* e)
{
    nodes[r_edge->to].excess+=e->residual;
    e->back_edge->residual+=e->residual;
    e->residual=0;
    if(nodes[r_edge->to].excess>0)
    {
        if(r_edge->to!=input->t)
            active.Enqueue(&active, &(r_edge->to));
    }
}

void simpleMaxflow(flow_graph* input)
{
    nodes = calloc(input->num_nodes, sizeof(node));
    nighbors = calloc(input->num_nodes, sizeof(nlist));
    active = new_nonconcurrent_ring_buffer(input->num_nodes*2, sizeof(uint32_t));
    in_graph = input;
    int i;
    for(i=0; i < input.num_nodes; i++)
    {
        nodes[i].id=i;
        nodes[i].lable=0;

        if ( input.s==i )
        {
            nodes[i].lable=input.num_nodes;
        }
        nodes[i].excess=0;
        
        nlist[i].capacity=6;
        nlist[i].size=0;
        nlist[i].list=calloc(6, sizeof(r_edge));

    }
    
    for(i=0; i < input.num_nodes; i++)
    {
        /* construct residual graph */
        int j;
        for(int j=i; j < input.num_nodes; j++)
        {
            uint32_t cap = getCap(&input, i, j);
            uint32_t cap_ = getCap(&input, j, i);
            if(cap==0 && cap_==0 )
                continue;
            r_edge* ji = &(nlist[j].list[nlist.size]);
            r_edge* ij = &(nlist[i].list[nlist.size]);

            nlist[j].list[nlist.size].from =j;
            nlist[j].list[nlist.size].to =i;
            nlist[j].list[nlist.size].residual = cap;
            nlist[j].list[nlist.size].back_edge = ij;
            nlist[j].size++;       
            nlist[i].list[nlist.size].from =i;
            nlist[i].list[nlist.size].to =j;
            nlist[i].list[nlist.size].residual = cap_;
            nlist[i].list[nlist.size].back_edge = ji;
            nlist[i].size++;
        }
    }
    /* push maximum flow flom sorce */

    for(i=0; i < nighbors[input->s].size; i++)
    {
        sorce_push(nighbors[input->s].list[i]);
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
        for(i=0; i < nighbors[curr].size; i++)
        {
            if(nighbors[curr_ptr].excess==0)
            {
                break;
            }
            push(nighbors[curr_ptr].list[i]);
            
        }
    }

    /* figure out actual flow */

}

void testflow()
{
    /* a test graph */
    flow_graph testgraph1;
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


}

