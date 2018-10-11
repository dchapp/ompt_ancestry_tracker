
/* OMPT callback for explicit task creation. 
 *
 */
static void
on_ompt_callback_task_create(
    ompt_data_t *encountering_task_data,
    const omp_frame_t *encountering_task_frame,
    ompt_data_t* new_task_data,
    int type,
    int has_dependences,
    const void *codeptr_ra)
{
#ifdef DEBUG
  printf("\nENTERING OMPT_TASK_CREATE\n");
  if(new_task_data->ptr) {
    printf("0: new_task_data initially not null\n");
  } 
#endif

  new_task_data->value = ompt_get_unique_id();
  char buffer[2048];
  format_task_type(type, buffer);

  //there is no parallel_begin callback for implicit parallel region
  //thus it is initialized in initial task
  if(type & ompt_task_initial)
  {
    ompt_data_t *parallel_data;
    ompt_get_parallel_info(0, &parallel_data, NULL);
#ifdef DEBUG
    if(parallel_data->ptr)
      printf("%s\n", "0: parallel_data initially not null");
#endif
    parallel_data->value = ompt_get_unique_id();
  }

  uint64_t task_id = new_task_data->value;
  uint64_t parent_task_id = encountering_task_data ? encountering_task_data->value : 0; 


  // Create a task object to represent this task
#ifdef DEBUG
  printf("\nCreating task object for task %lu\n", task_id);
#endif
  
  Task * t = new Task(task_id, parent_task_id, TaskType::Explicit, codeptr_ra);

  // Handle initial task case
  if (type == 1) {
    t->set_as_initial_task();
    tool_data_ptr->initial_task_id = task_id; 
  }

  register_task(t, tool_data_ptr); 

  
}
