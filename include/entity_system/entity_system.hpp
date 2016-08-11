#ifndef ENTITY_SYSTEM_ENTITY_SYSTEM_HPP
# define ENTITY_SYSTEM_ENTITY_SYSTEM_HPP

# include <entity_system/forwards.hpp>
# include <entity_system/event_dispatcher.hpp>
# include <entity_system/segment.hpp>

# include <bitset>
# include <map>
# include <limits>

namespace entity_system
{
  namespace detail
  {
    template <class T, class Tuple> struct components_index;

    template <class T, class... Types> struct components_index<T, std::tuple<T, Types...>> {
        static const std::size_t value = 0;
    };

    template <class T, class U, class... Types> struct components_index<T, std::tuple<U, Types...>> {
        static const std::size_t value = 1 + components_index<T, std::tuple<Types...>>::value;
    };

    template <class ...> struct delete_all_components;
    template <class E, class ... Tuples> struct delete_all_components <E, std::tuple<Tuples...>>
    {
        static void process(E& entity)
        {
          int tmp[] = {(entity.template delete_component<Tuples>(),0)...};
          (void)tmp;
        }
    };
  }

  template <class ... Events, class ... Components> class world<std::tuple<Events...>, std::tuple<Components...>>
  {
    public:
      typedef world                              self_type;
      typedef entity<self_type>                  entity_type;
      typedef entity_manager<self_type>          entity_manager_type;
      typedef system_manager<self_type>          system_manager_type;
      typedef dispatcher<Events...>              dispatcher_type;
      typedef std::bitset<sizeof...(Components)> component_mask_type;
      typedef std::tuple<Components...>          components_type;

      world()
        : entity_manager_(*this)
        , system_manager_(*this)
      {
      }

      template <class ... C> static constexpr component_mask_type get_component_mask()
      {
        component_mask_type ret;
        for(std::size_t index : {detail::components_index<C, components_type>::value...})
        {
          ret[index] = true;
        }
        return ret;
      }

      entity_manager_type& get_entity_manager() { return entity_manager_; }
      const entity_manager_type& get_entity_manager() const { return entity_manager_; }
      system_manager_type& get_system_manager() { return system_manager_; }
      const system_manager_type& get_system_manager() const { return system_manager_; }

    private:
      entity_manager_type entity_manager_;
      system_manager_type system_manager_;
  };

  template <class World> class entity
  {
    public:
      typedef entity                                   self_type;
      typedef World                                    world_type;
      typedef typename world_type::entity_manager_type entity_manager_type;
      typedef typename world_type::component_mask_type component_mask_type;
      typedef typename world_type::components_type     components_type;

      friend entity_manager_type;

      entity(entity_manager_type& em)
        : entity_manager_(em)
      {
      }

      ~entity()
      {
        delete_all_components();
      }

      entity_id_type get_id() const;
      const component_mask_type& get_component_mask() const { return mask_component_; }

      template <class T> T*   get_component();
      template <class T, class ... ARGS> T* new_component(ARGS && ... args);
      template <class T> void delete_component();
      void delete_all_components()
      {
        detail::delete_all_components<self_type, components_type>::process(*this);
      }

    private:
      component_mask_type  mask_component_;
      entity_manager_type& entity_manager_;
  };

  template <class ... Events, class ... Components> class entity_manager<world<std::tuple<Events...>, std::tuple<Components...>>>
  {
    public:
      typedef world<std::tuple<Events...>, std::tuple<Components...>> world_type;
      typedef typename world_type::entity_type                        entity_type;
      typedef typename world_type::component_mask_type                component_mask_type;

      friend entity_type;

      entity_manager(world_type& w)
        : world_(w)
      {
      }

      ~entity_manager()
      {
      }

      entity_type* new_entity();
      void    delete_entity(const entity_type& e);

      template <class ... C, class F> void for_entities_with(F && functor)
      {
        component_mask_type mask = world_type::template get_component_mask<C...>();
        for(entity_wrapper_type* wrapper : entities_)
        {
          entity_type& entity = wrapper->data();
          if((entity.get_component_mask() & mask) == mask)
          {
            functor(entity);
          }
        }
      }

    protected:
      entity_id_type get_id_(const entity_type& e);
      template <class T> T*  get_component_(const entity_type& e);
      template <class T, class ... ARGS> T* new_component_(const entity_type& e, ARGS && ... args);
      template <class T> void delete_component_(const entity_type& e);

      template <class I, class T> class wrapper
      {
        public:
          typedef wrapper self_type;
          typedef I       id_type;
          typedef T       data_type;

          template <class ... ARGS> wrapper(ARGS&& ... args)
            : id_(0)
            , data_(std::forward<ARGS>(args)...)
          {
          }

          static const self_type& to_wrapper(const data_type& data)
          {
            return *(const self_type*)((const uint8_t*)&data - ptr_id_2_data_());
          }

          data_type& data()
          {
            return data_;
          }

          const data_type& data() const
          {
            return data_;
          }

          id_type& id()
          {
            return id_;
          }

          const id_type& id() const
          {
            return id_;
          }

        protected:
          static constexpr size_t ptr_id_2_data_()
          {
            wrapper* tmp = 0;
            void* p1 = tmp;
            void* p2 = &tmp->data_;
            return ((size_t)p2-(size_t)p1);
          }

        private:
          id_type   id_;
          data_type data_;
      };

      template <class Component> class component_manager
      {
        public:
          typedef Component component_type;

          template <class ... ARGS> component_type* acquire(const entity_type&e, ARGS && ...args)
          {
            entity_id_type entity_id = e.get_id();
            if(entity_id + 1 > mapping_.size())
            {
              mapping_.resize(entity_id + 1, invalid_());
            }
            component_id_type& component_id = mapping_[entity_id];

            wrapper_type* wrapper = nullptr;
            data_id_type  intern_component_id(0);

            std::tie(wrapper, intern_component_id) = data_.acquire(std::forward<ARGS>(args)...);
            if(wrapper)
            {
              component_id  = intern_component_id;
              wrapper->id() = component_id;
            }

            return (wrapper ? &wrapper->data() : nullptr);
          }

          void release(const entity_type& e)
          {
            entity_id_type entity_id = e.get_id();
            if(entity_id < mapping_.size())
            {
              component_id_type& component_id = mapping_[entity_id];
              if(valid_(component_id))
              {
                data_.release(component_id);
                component_id = invalid_();
              }
            }
          }

          component_type* get(const entity_type& e)
          {
            component_type* ret = nullptr;
            entity_id_type entity_id = e.get_id();
            if(entity_id < mapping_.size())
            {
              const component_id_type& component_id = mapping_[entity_id];
              if(valid_(component_id))
              {
                ret = &data_.get(component_id)->data();
              }
            }
            return ret;
          }

          const component_type* get(const entity_type& e) const
          {
            const component_type* ret = nullptr;
            entity_id_type entity_id = e.get_id();
            if(entity_id < mapping_.size())
            {
              const component_id_type& component_id = mapping_[entity_id];
              if(valid_(component_id))
              {
                ret = &data_.get(component_id)->data();
              }
            }
            return ret;
          }

        protected:
          typedef wrapper<component_id_type, component_type>  wrapper_type;
          typedef dynamic_segment<wrapper_type, 8>            data_type;
          typedef typename data_type::id_type                 data_id_type;
          typedef std::vector<component_id_type>              mapping_component_id_type;

          static constexpr bool valid_(component_id_type id)
          {
            return id != invalid_();
          }

          static constexpr component_id_type invalid_()
          {
            return std::numeric_limits<component_id_type>::max();
          }

        private:
          data_type                 data_;
          mapping_component_id_type mapping_;
      };

      typedef wrapper<entity_id_type, entity_type>         entity_wrapper_type;
      typedef dynamic_segment<entity_wrapper_type, 8>      entities_type;
      typedef std::tuple<component_manager<Components>...> components_type;

    private:
      world_type&     world_;
      components_type components_;
      entities_type   entities_;
  };

  class system
  {
    public:
      inline virtual ~system() {}
  };

  template <class World> class system_manager
  {
    public:
      typedef World                                world_type;
      typedef typename world_type::dispatcher_type dispatcher_type;

      system_manager(world_type& w)
        : world_(w)
        , next_id_(0)
      {
      }

      ~system_manager()
      {
      }

      void process()
      {
        dispatcher_.dispatch();
      }

      system_id_type add_system(system* s)
      {
        systems_[next_id_] = std::unique_ptr<system>(s);
        return next_id_++;
      }
      void delete_system(system_id_type id) { systems_.erase(id); }

      dispatcher_type& get_dispatcher() { return dispatcher_; }
      const dispatcher_type& get_dispatcher() const { return dispatcher_; }

    private:
      typedef std::map<system_id_type, std::unique_ptr<system>> systems_type;

      world_type&      world_;
      dispatcher_type  dispatcher_;
      systems_type     systems_;
      system_id_type   next_id_;
  };

  // impl. template

  // entity
  template <class World> entity_id_type entity<World>::get_id() const
  {
    return entity_manager_.get_id_(*this);
  }

  template <class World> template <class T> T* entity<World>::get_component()
  {    
    static const size_t pos = detail::components_index<T, components_type>::value;
    if(mask_component_[pos])
    {
      return entity_manager_.template get_component_<T>(*this);
    }
    else
    {
      return nullptr;
    }
  }

  template <class World> template <class T, class ... ARGS> T* entity<World>::new_component(ARGS && ... args)
  {
    static const size_t pos = detail::components_index<T, components_type>::value;
    T* ret = nullptr;
    if(!mask_component_[pos])
    {
      ret = entity_manager_.template new_component_<T>(*this, std::forward<ARGS>(args)...);
      if(ret)
      {
        mask_component_[pos] = true;
      }
    }
    return ret;
  }

  template <class World> template <class T> void entity<World>::delete_component()
  {
    static const size_t pos = detail::components_index<T, components_type>::value;
    if(mask_component_[pos])
    {
      entity_manager_.template delete_component_<T>(*this);
      mask_component_[pos] = false;
    }
  }

  // entity_manager
  template <class ... Events, class ... Components> entity_id_type entity_manager<world<std::tuple<Events...>, std::tuple<Components...>>>::get_id_(const entity_type& e)
  {
    const entity_wrapper_type& wrapper =  entity_wrapper_type::to_wrapper(e);
    return wrapper.id();
  }

  template <class ... Events, class ... Components> typename entity_manager<world<std::tuple<Events...>, std::tuple<Components...>>>::entity_type* entity_manager<world<std::tuple<Events...>, std::tuple<Components...>>>::new_entity()
  {
    std::pair<entity_wrapper_type*, typename entities_type::id_type> ret = entities_.acquire(std::ref(*this));
    ret.first->id() = ret.second;
    return &ret.first->data();
  }

  template <class ... Events, class ... Components> void entity_manager<world<std::tuple<Events...>, std::tuple<Components...>>>::delete_entity(const entity_type& e)
  {
    entities_.release(get_id_(e));
  }

  template <class ... Events, class ... Components> template <class T> T* entity_manager<world<std::tuple<Events...>, std::tuple<Components...>>>::get_component_(const entity_type& e)
  {
    typedef component_manager<T> manager_type;
    manager_type& manager = std::get<manager_type>(components_);    
    return manager.get(e);
  }

  template <class ... Events, class ... Components> template <class T, class ... ARGS> T* entity_manager<world<std::tuple<Events...>, std::tuple<Components...>>>::new_component_(const entity_type& e, ARGS && ... args)
  {
    typedef component_manager<T> manager_type;
    manager_type& manager = std::get<manager_type>(components_);
    return manager.acquire(e, std::forward<ARGS>(args)...);
  }

  template <class ... Events, class ... Components> template <class T> void entity_manager<world<std::tuple<Events...>, std::tuple<Components...>>>::delete_component_(const entity_type& e)
  {
    typedef component_manager<T> manager_type;
    manager_type& manager = std::get<manager_type>(components_);
    manager.release(e);
  }
}

#endif
