
/* OMPT callback for parallel region creation 
 * When we enter a parallel region, the tool should add a vertex to represent
 * the region to the task ancestry tree since the parallel region is effectively
 * the parent of the implicit tasks that are generated upon entry.  
 */
static void
on_ompt_callback_parallel_begin(
  ompt_data_t *encountering_task_data,
  const omp_frame_t *encountering_task_frame,
  ompt_data_t* parallel_data,
  uint32_t requested_team_size,
  ompt_invoker_t invoker,
  const void *codeptr_ra)
{
#ifdef DEBUG
  printf("\nENTERING PARALLEL_BEGIN\n"); 
  if(parallel_data->ptr) {
    printf("0: parallel_data initially not null\n");
  }
#endif 

  // Get ID of this parallel region and its parent 
  parallel_data->value = ompt_get_unique_id();
  uint64_t parent_task_id = encountering_task_data->value;
  uint64_t parallel_region_id = parallel_data->value; 

#ifdef DEBUG
  std::cout << "Creating ParallelRegion object" << std::endl;
#endif
  ParallelRegion * region = new ParallelRegion(parallel_region_id, 
                                               parent_task_id, 
                                               requested_team_size,
                                               codeptr_ra);
  register_parallel_region(region, tool_data_ptr); 



}





/* OMPT callback for parallel region completion 
 *
 */
static void
on_ompt_callback_parallel_end(
  ompt_data_t *parallel_data,
  ompt_data_t *encountering_task_data,
  ompt_invoker_t invoker,
  const void *codeptr_ra)
{

#ifdef DEBUG
  printf("\nENTERING PARALLEL_END\n"); 
#endif

  uint64_t parallel_id = parallel_data->value;

}
