#ifndef H_FOWLTALK_VTABLE
#define H_FOWLTALK_VTABLE

enum ft_vtable_slot_type
{
  ftvs_static_slot = 1 << 0,
  ftvs_parent_slot = 1 << 1,
  ftvs_setter_slot = 1 << 2,

  ftvs_data_slot = 0,
  ftvs_static_parent = ftvs_static_slot | ftvs_parent_slot,
};

enum
{
  ftvs__mask = (1 << 3) - 1
};

enum ft_vtable_flags
{
  ftvf_activatable_method = 1 << 0 // flag is set for method objects but not read by the vm, instead the presence of a code object in the code field makes it activable
};

struct ft_vtable_slot_desc
{
  struct ft_string* name;
  enum ft_vtable_slot_type type;
  oop value;
};

struct ft_vtable_slot;

void ft_vtable_slot_init(struct ft_vtable_slot* slot, enum ft_vtable_slot_type slot_type, struct ft_string* name, oop value);
enum ft_vtable_slot_type ft_vtable_slot_type(struct ft_vtable_slot* slot);
struct ft_string* ft_vtable_slot_name(struct ft_vtable_slot* slot);
oop ft_vtable_slot_value(struct ft_vtable_slot* slot);
struct ft_vtable_slot* ft_vtable_slot_next(struct ft_vtable_slot* slot);

void ft_vtable_slot_desc_init(struct ft_vtable_slot_desc* slot, enum ft_vtable_slot_type slot_type, struct ft_string* name, oop value);

struct ft_vtable* ft_new_method_vtable(struct ft_allocator* allocator, struct ft_vtable* vtable, int slot_count, struct ft_vtable_slot_desc* slots, oop code);
struct ft_vtable* ft_new_vtable(struct ft_allocator* allocator, struct ft_vtable* vtable, int slot_count, struct ft_vtable_slot_desc* slots, int base_size);
struct ft_vtable* ft_object_vtable(oop object);
struct ft_object* ft_vtable_instantiate(struct ft_allocator* allocator, struct ft_vtable* vtable);
struct ft_vtable_slot* ft_vtable_lookup(struct ft_vtable* vt, struct ft_string* selector);


struct ft_vtable_slot* ft_vtable_slots_begin(struct ft_vtable* vt);
struct ft_vtable_slot* ft_vtable_slots_end(struct ft_vtable* vt);
oop* ft_vtable_static_parents_begin(struct ft_vtable* vt);
oop* ft_vtable_static_parents_end(struct ft_vtable* vt);
oop* ft_vtable_parents_begin(struct ft_vtable* vt, oop value);
oop* ft_vtable_parents_end(struct ft_vtable* vt, oop value);
oop* ft_vtable_data_slots_begin(struct ft_vtable* vt, oop value);
oop* ft_vtable_data_slots_end(struct ft_vtable* vt, oop value);

int ft_vtable_slot_capacity(struct ft_vtable* vt);
int ft_vtable_base_size(struct ft_vtable* vt);

oop ft_vtable_get_code(struct ft_vtable* vt);
oop ft_vtable_set_code(struct ft_vtable* vt, oop code);

int ft_vtable_decompose_slots(struct ft_vtable* vt, int buffer_size, struct ft_vtable_slot_desc* slots);


void ft_vtable_debug_print(struct ft_vtable* vt);
void ft_vtable_each_slot(struct ft_vtable* vt, void(*callback)(enum ft_vtable_slot_type, struct ft_string*, oop));
void ft_vtable_print_stats(struct ft_vtable* vt);

#endif
