#include "dp_base.h"

typedef struct dp_baselist_t {
 dp_base_t **list;
 dp_bool_t valid;
 uint32_t size;
 uint32_t count;
} dp_baselist_t;

/* creates list and initializes */
dp_baselist_t *dp_baselist_create(uint32_t s);

/* initializes only */
void dp_baselist_init(dp_baselist_t *b, uint32_t s);

/* deinitializes only 
 * does not destroy and free modules that may have been added. */
void dp_baselist_deinit(dp_baselist_t *b);

/* desttroy and frees object. 
 * does not destroy and free modules that may have been added. */
void dp_baselist_destroy(dp_baselist_t *b);

/*
// cleans up and destroy all the elems of the list --
//  this may nto be what you want if not all the elements
//  were allocated on the heap, etc.
*/
void dp_baselist_destroy_elems(dp_baselist_t *bl);

/* add new element */
void dp_baselist_add(dp_baselist_t *bl, dp_base_t *b);

/* call the functions  */
void dp_baselist_prerun(dp_baselist_t *bl);
void dp_baselist_run(dp_baselist_t *bl, uint32_t rmask);
void dp_baselist_postrun(dp_baselist_t *bl);


