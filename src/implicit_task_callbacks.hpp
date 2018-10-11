#include "Task.hpp" 



/* OMPT callback for implicit task creation. 
 *
 */
static void
on_ompt_callback_implicit_task(
    ompt_scope_endpoint_t endpoint,
    ompt_data_t *parallel_data,
    ompt_data_t *task_data,
    unsigned int team_size,
    unsigned int thread_num)
{
  // Implicit task creation
  if (endpoint == ompt_scope_begin) {
#ifdef DEBUG
    printf("\nENTERING OMPT_IMPLICIT_TASK_CREATION\n");
#endif 

#ifdef DEBUG
    if(task_data->ptr) {
      printf("%s\n", "0: task_data initially not null");
    }
#endif 
    task_data->value = ompt_get_unique_id();


    uint64_t task_id = task_data->value;
    uint64_t parallel_region_id = parallel_data->value;
    uint64_t thread_id = thread_num;

    // Create a task object to represent this task
#ifdef DEBUG
    printf("\nCreating task object for task %lu\n", task_id);
#endif
    void * codeptr_ra = NULL; 
    Task * task_ptr = new Task(task_id, parallel_region_id, TaskType::Implicit, codeptr_ra);
    register_task(task_ptr, tool_data_ptr); 
    register_task_as_child_of_parallel_region(parallel_region_id, task_id, tool_data_ptr);

  // Implicit task completion
  } else if (endpoint == ompt_scope_end) {
#ifdef DEBUG
    printf("\nENTERING OMPT_IMPLICIT_TASK_COMPLETION\n");
#endif 


    // Warning! Trying to get the parallel region ID here will segfault 
    uint64_t task_id = task_data->value;
    //complete_task(task_id, tool_data_ptr); 

  
  } else {
#ifdef DEBUG
    printf("Endpoint not equal to ompt_scope_begin or ompt_scope_end "
           "encountered in implicit_task callback\n"); 
#endif 
  }

}














