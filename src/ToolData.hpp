#ifndef TOOL_DATA_H
#define TOOL_DATA_H

#include "boost/thread/mutex.hpp" 
#include "boost/thread/locks.hpp" 
#include <unordered_map> 
#include <inttypes.h> 

#include "tbb/tbb.h" 
#include "tbb/concurrent_unordered_map.h"

#include "Task.hpp" 
#include "ParallelRegion.hpp"
#include "Tree.hpp" 

/******************************************************************************\
 * The tool_data_t defines the data the tool needs to determine parent-child
 * and dependency relationships between tasks, thereby building up the 
 * parent-child tree and dependence DAG. 
\******************************************************************************/
typedef struct tool_data {
  boost::mutex mtx; 
  uint64_t initial_task_id;
  boost::mutex print_mtx; 

  // We will always need to be able to look up task objects by their numerical
  // IDs in order to check things like whether the task has been schedule, which
  // thread worked on it last etc. 
  std::unordered_map<uint64_t, Task*> id_to_task; 
  boost::mutex id_to_task_mtx; 

  // We will also want to be able to look up parallel region objects 
  std::unordered_map<uint64_t, ParallelRegion*> id_to_parallel_region;
  boost::mutex id_to_parallel_region_mtx; 

  // We will eventually construct a tree that contains the ancestry 
  // relationships between parallel regions, implicit tasks, and explicit tasks
  std::unordered_map<uint64_t, vertex_t> id_to_vertex;
  boost::mutex id_to_vertex_mtx;
  tree_t tree;
  boost::mutex tree_mtx; 


#ifdef TRACK_ANCESTRY
  // Mappings for tracking task-to-task ancestry relationships
  std::unordered_map<uint64_t, uint64_t> task_id_to_parent_task_id;
  std::unordered_map<uint64_t, std::vector<uint64_t> * > parent_task_id_to_task_id;

  // Mutexes for task-to-task ancestry relationship mappings
  boost::mutex task_id_to_parent_task_id_mtx; 
  boost::mutex parent_task_id_to_task_id_mtx; 

  // Mappings for tracking task-to-parallel-region ancestry relationships
  std::unordered_map<uint64_t, uint64_t> parent_task_id_to_child_parallel_region_id;
  std::unordered_map<uint64_t, uint64_t> child_parallel_region_id_to_parent_task_id;
  std::unordered_map<uint64_t, uint64_t> child_task_id_to_parent_parallel_region_id;
  std::unordered_map<uint64_t, std::vector<uint64_t> * > parent_parallel_region_id_to_child_task_id;

  // Mutexes for task-to-parallel-region ancestry relationship mappings 
  boost::mutex parent_task_id_to_child_parallel_region_id_mtx; 
  boost::mutex child_parallel_region_id_to_parent_task_id_mtx;
  boost::mutex child_task_id_to_parent_parallel_region_id_mtx;
  boost::mutex parent_parallel_region_id_to_child_task_id_mtx; 
#endif 

} tool_data_t; 


void register_task_as_child_of_parallel_region(uint64_t region_id, 
                                               uint64_t task_id, 
                                               tool_data_t * tool_data) {
  auto id_to_region = &(tool_data->id_to_parallel_region);
  auto id_to_task = &(tool_data->id_to_task);
  auto region_search = id_to_region->find(region_id);
  auto task_search = id_to_task->find(task_id);
  if (region_search != id_to_region->end() && task_search != id_to_task->end()) {
    region_search->second->add_child(task_search->second);    
  }
}

/* 
 *
 */
void register_parallel_region(ParallelRegion * new_region, tool_data_t * tool_data)
{
  boost::lock_guard<boost::mutex> lock(tool_data->id_to_parallel_region_mtx);
  auto id_to_region = &(tool_data->id_to_parallel_region);
  uint64_t region_id = new_region->get_id();
  auto insert_result = id_to_region->insert({region_id, new_region});
#ifdef DEBUG
  if (insert_result.second == false) {
    printf("\t- Region %lu already has a ParallelRegion object associated with it\n",
           region_id);
  } else {
    printf("\t- Associating region %lu with ParallelRegion object:\n", region_id);
  }
#endif 
}


/* Associate a task ID to a pointer to the corresponding task object.
 * This is the only function that writes to the id_to_task map. 
 */
void register_task(Task * new_task, tool_data_t * tool_data) 
{
  //// Acquire exclusive access to the id_to_task map
  //boost::lock_guard<boost::mutex> lock(tool_data->id_to_task_mtx);
  //// Insert the new task into the id_to_task map
  //std::unordered_map<uint64_t, Task*> * id_to_task = &(tool_data->id_to_task);
  //std::pair<std::unordered_map<uint64_t, Task*>::iterator, bool> insert_result;
  //uint64_t task_id = new_task->get_id();
  //std::pair<uint64_t, Task*> kvp = {task_id, new_task}; 
  //insert_result = id_to_task->insert(kvp);

  // Get a pointer to the Task ID --> Task Object map
  auto id_to_task = &(tool_data->id_to_task);
  auto insert_result = id_to_task->insert( {new_task->get_id(), new_task} );


#ifdef DEBUG
  if (insert_result.second == false) {
    printf("\t- Task %lu already has a task object associated with it.\n",
           new_task->get_id());
  } else {
    printf("\t- Task %lu successfully registered.\n", new_task->get_id());
    new_task->print();
  }
#endif 
}

/* Mark a task as complete */
void complete_task(uint64_t id, tool_data_t * tool_data) {
  std::unordered_map<uint64_t, Task*> * id_to_task = &(tool_data->id_to_task);
  auto search = id_to_task->find(id);
  if (search != id_to_task->end()) {
    search->second->change_state(TaskState::Completed); 
  } else {
#ifdef DEBUG
    printf("\t- ID %lu does not have a task object associated with it\n", id);
#endif
  }
}
#endif // TOOL_DATA_H
