#include <iostream>
#include <vector> 
#include <unordered_map>
//#include <memory> // make_unique
#include <csignal> // Need signal handling to dump tree on interrupt
#include <inttypes.h>
#include <stdio.h>

#include "omp.h"
#include "ompt.h"

/******************************************************************************\
 * Definitions for tool configuration
\******************************************************************************/
#define DEBUGS
#define BUILD_TREE_ON_FINALIZE
#define PRINT_SUMMARY_TASK_REGIONS 
#define PRINT_SUMMARY_PARALLEL_REGIONS

#include "OMPT_helpers.hpp" 


#include "ToolData.hpp" 
#include "Task.hpp" 
#include "Tree.hpp"



static tool_data_t* tool_data_ptr;


#include "implicit_task_callbacks.hpp"
#include "explicit_task_creation.hpp" 
#include "parallel_region_callbacks.hpp" 

void add_parallel_region_vertex(ParallelRegion * pr) {
  // Synchronize access to the tree and the id_to_vertex map
  boost::lock_guard<boost::mutex> tree_lock(tool_data_ptr->tree_mtx);
  boost::lock_guard<boost::mutex> map_lock(tool_data_ptr->id_to_vertex_mtx);
  // First create vertex properties 
  const auto vp = construct_vprops(VertexType::ParallelRegion,
                                   pr->get_id(),
                                   pr->get_codeptr_ra());
  // Add a new vertex to the tree
  const vertex_t v = boost::add_vertex(vp, tool_data_ptr->tree);
  // Update the id_to_vertex map
  auto id_to_vertex = &(tool_data_ptr->id_to_vertex);
  auto res = id_to_vertex->insert( {pr->get_id(), v} ); 
}

void add_task_vertex(Task * t) {
  // Synchronize access to the tree and the id_to_vertex map
  boost::lock_guard<boost::mutex> tree_lock(tool_data_ptr->tree_mtx);
  boost::lock_guard<boost::mutex> map_lock(tool_data_ptr->id_to_vertex_mtx);
  // First create vertex properties 
  VertexType vt;
  if (t->get_type() == TaskType::Explicit) {
    vt = VertexType::ExplicitTask;
  } else {
    vt = VertexType::ImplicitTask;
  }
  const auto vp = construct_vprops(vt, t->get_id(), t->get_codeptr_ra());
  // Add a new vertex to the tree
  const vertex_t v = boost::add_vertex(vp, tool_data_ptr->tree);
  // Update the id_to_vertex map
  auto id_to_vertex = &(tool_data_ptr->id_to_vertex);
  auto res = id_to_vertex->insert( {t->get_id(), v} ); 
}


void add_vertices(tool_data_t * tool_data) {
  // Add vertices for parallel regions 
  auto id_to_region = tool_data->id_to_parallel_region;
  for (auto e : id_to_region) {
     add_parallel_region_vertex(e.second);   
  } 
  // Add vertices for tasks
  auto id_to_task = tool_data->id_to_task;
  for (auto e : id_to_task) {
     add_task_vertex(e.second);   
  } 
}

void link_region_with_parent(ParallelRegion * pr) {
  uint64_t region_id = pr->get_id();
  uint64_t parent_id = pr->get_parent_id(); 
  auto id_to_vertex = &(tool_data_ptr->id_to_vertex);
  auto region_search = id_to_vertex->find(region_id);
  auto parent_search = id_to_vertex->find(parent_id);
  if (region_search != id_to_vertex->end() && parent_search != id_to_vertex->end()) {
    vertex_t region_vertex = region_search->second;
    vertex_t parent_vertex = parent_search->second;
    boost::add_edge(parent_vertex, region_vertex, tool_data_ptr->tree); 
  }
}

void link_task_with_parent(Task * t) {
  // Exit early if this is the initial task
  if (t->is_initial()) {
    return;
  } 
  uint64_t task_id = t->get_id();
  uint64_t parent_id = t->get_parent_id(); 
  auto id_to_vertex = &(tool_data_ptr->id_to_vertex);
  auto task_search = id_to_vertex->find(task_id);
  auto parent_search = id_to_vertex->find(parent_id);
  if (task_search != id_to_vertex->end() && parent_search != id_to_vertex->end()) {
    vertex_t task_vertex = task_search->second;
    vertex_t parent_vertex = parent_search->second;
    boost::add_edge(parent_vertex, task_vertex, tool_data_ptr->tree); 
  }
}


void add_edges(tool_data_t * tool_data) { 
  auto id_to_region = tool_data->id_to_parallel_region;
  for (auto e : id_to_region) {
     link_region_with_parent(e.second);   
  } 
  auto id_to_task = tool_data->id_to_task;
  for (auto e : id_to_task) {
     link_task_with_parent(e.second);   
  } 
} 


void build_tree(tool_data_t * tool_data) {
  add_vertices(tool_data);
  add_edges(tool_data); 
}

void write_tree(tree_t tree) {
  // Get dotfile name for task tree visualization from environment
  char * env_var;
  std::string tree_dotfile;
  env_var = getenv("TASK_TREE_DOTFILE");
  if (env_var == NULL) {
    printf("TASK_TREE_DOTFILE not specified\n");
    tree_dotfile = "./tree.dot"; 
  } else {
    tree_dotfile = env_var;
  }
  // Open the stream to the dotfile for the task ancestry tree
  std::ofstream out(tree_dotfile);
  // Construct custom vertex writer
  auto tree_vw = make_vertex_writer(
    boost::get(&vertex_properties::vertex_id, tree),
    boost::get(&vertex_properties::vertex_type, tree),
    boost::get(&vertex_properties::color, tree),
    boost::get(&vertex_properties::shape, tree),
    boost::get(&vertex_properties::status, tree),
    boost::get(&vertex_properties::codeptr_ra, tree),
    boost::get(&vertex_properties::dependences, tree)
  );
  // Write out the task ancestry tree 
  boost::write_graphviz(out, tree, tree_vw); 

}

/******************************************************************************\
 * This function writes the parent-child tree and dependence DAG out to files
 * upon catching SIGINT or SIGSEGV 
\******************************************************************************/
void signal_handler(int signum) {

#ifdef DEBUG
  printf("\n\nTool caught signal: %d\n", signum);
#endif
  
  // Build the tree
  build_tree(tool_data_ptr);
  tree_t task_tree = tool_data_ptr->tree;

  std::cout << "Number of vertices in task tree: " << boost::num_edges(task_tree) << std::endl;

  // Write the tree
  write_tree(task_tree); 

  exit(signum);
}



int ompt_initialize(ompt_function_lookup_t lookup,
                    ompt_data_t * tool_data)
{
  // Instantiate the data structure that all the callbacks will refer to 
  tool_data->ptr = tool_data_ptr = new tool_data_t();

  // OMPT boilerplate 
  ompt_set_callback = (ompt_set_callback_t) lookup("ompt_set_callback");
  ompt_get_callback = (ompt_get_callback_t) lookup("ompt_get_callback");
  ompt_get_state = (ompt_get_state_t) lookup("ompt_get_state");
  ompt_get_task_info = (ompt_get_task_info_t) lookup("ompt_get_task_info");
  ompt_get_thread_data = (ompt_get_thread_data_t) lookup("ompt_get_thread_data");
  ompt_get_parallel_info = (ompt_get_parallel_info_t) lookup("ompt_get_parallel_info");
  ompt_get_unique_id = (ompt_get_unique_id_t) lookup("ompt_get_unique_id");
  ompt_get_num_procs = (ompt_get_num_procs_t) lookup("ompt_get_num_procs");
  ompt_get_num_places = (ompt_get_num_places_t) lookup("ompt_get_num_places");
  ompt_get_place_proc_ids = (ompt_get_place_proc_ids_t) lookup("ompt_get_place_proc_ids");
  ompt_get_place_num = (ompt_get_place_num_t) lookup("ompt_get_place_num");
  ompt_get_partition_place_nums = (ompt_get_partition_place_nums_t) lookup("ompt_get_partition_place_nums");
  ompt_get_proc_id = (ompt_get_proc_id_t) lookup("ompt_get_proc_id");
  ompt_enumerate_states = (ompt_enumerate_states_t) lookup("ompt_enumerate_states");
  ompt_enumerate_mutex_impls = (ompt_enumerate_mutex_impls_t) lookup("ompt_enumerate_mutex_impls");

  ompt_get_unique_id = (ompt_get_unique_id_t) lookup("ompt_get_unique_id"); 

  // Determine which OMPT callbacks the tool will invoke 
  register_callback(ompt_callback_task_create);
  register_callback(ompt_callback_implicit_task);
  register_callback(ompt_callback_parallel_begin);
  register_callback(ompt_callback_parallel_end);

  // Register signal handlers for graph visualization 
  signal(SIGINT, signal_handler); 

  printf("\n\n\n");
  printf("=================================================================\n");
  printf("====================== OMPT_INITIALIZE ==========================\n");
  printf("=================================================================\n");
  printf("\n\n\n");

  // Returning 1 instead of 0 lets the OpenMP runtime know to load the tool 
  return 1;
}


void ompt_finalize(ompt_data_t *tool_data)
{
  printf("\n\n\n");
  printf("=================================================================\n");
  printf("======================= OMPT_FINALIZE ===========================\n");
  printf("=================================================================\n");
  printf("\n\n\n");

#ifdef PRINT_SUMMARY_TASK_REGIONS
  printf("Tasks:\n");
  for (auto e : tool_data_ptr->id_to_task) {
    e.second->print(2); 
    printf("\n"); 
  }
#endif

#ifdef PRINT_SUMMARY_PARALLEL_REGIONS
  printf("Parallel Regions:\n");
  for (auto e : tool_data_ptr->id_to_parallel_region) {
    e.second->print(2); 
    printf("\n"); 
  }
#endif
  
  // Build the tree
  build_tree(tool_data_ptr);
  tree_t task_tree = tool_data_ptr->tree;

  std::cout << "Number of vertices in task tree: " << boost::num_vertices(task_tree) << std::endl;

  write_tree(task_tree); 

  printf("0: ompt_event_runtime_shutdown\n"); 

  // Delete tool data 
  for ( auto e : tool_data_ptr->id_to_task ) {
    delete e.second; 
  }
  for ( auto e : tool_data_ptr->id_to_parallel_region ) {
    delete e.second; 
  }
  delete (tool_data_t*)tool_data->ptr; 
}



/* "A tool indicates its interest in using the OMPT interface by providing a 
 * non-NULL pointer to an ompt_fns_t structure to an OpenMP implementation as a 
 * return value from ompt_start_tool." (OMP TR4 4.2.1)
 */
ompt_start_tool_result_t* ompt_start_tool(unsigned int omp_version,
                                          const char *runtime_version)
{
  static ompt_start_tool_result_t ompt_start_tool_result = {&ompt_initialize,
                                                            &ompt_finalize, 
                                                            0};
  return &ompt_start_tool_result;
}
