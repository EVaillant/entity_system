#ifndef ENTITY_SYSTEM_FORWARDS_HPP
# define ENTITY_SYSTEM_FORWARDS_HPP

# include <cstdint>
# include <stddef.h>

namespace entity_system
{
  typedef uint32_t component_id_type;
  typedef uint32_t system_id_type;
  typedef uint32_t entity_id_type;

  template <class E> class listener;
  class event_handler;
  class event_handler_deleter;
  template <class O, class ... E> class event_dispatcher;

  template <class Seg> class segment_iterator;
  template <class T, size_t S> class segment;
  template <class T, size_t S> class dynamic_segment;

  template <class...> class world;
  template<class> class entity;
  template<class> class entity_manager;
  class system;
  template<class> class system_manager;
}

#endif
