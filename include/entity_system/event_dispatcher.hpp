#ifndef ENTITY_SYSTEM_EVENT_DISPATCHER_HPP
# define ENTITY_SYSTEM_EVENT_DISPATCHER_HPP

# include <entity_system/forwards.hpp>

# include <memory>
# include <vector>
# include <utility>

namespace entity_system
{
  template <class E> class listener
  {
    public:
      virtual ~listener() {}
      virtual void handle(E& event) = 0;
  };

  class event_handler
  {
    public:
      virtual ~event_handler() {}

      virtual void process() = 0;
      virtual void dispose() = 0;
  };

  class event_handler_deleter
  {
    public:
      typedef std::unique_ptr<event_handler, event_handler_deleter> ptr_type;
      inline void operator()(event_handler* t)
      {
        t->dispose();
      }
  };

  template <class O> class event_dispatcher<O>
  {
  };

  template <class O, class E> class event_dispatcher<O, E>
  {
    protected:
      typedef E                              event_type;
      typedef O                              owner_type;
      typedef listener<event_type>           listener_type;
      typedef std::vector<listener_type*>    listeners_type;

      void push_(const event_type& event)
      {
        static_cast<owner_type*>(this)->events_.push_back(make_handler_(event));
      }

      void push_(event_type&& event)
      {
        static_cast<owner_type*>(this)->events_.push_back(make_handler_(std::forward<event_type>(event)));
      }

      template <class ...ARGS> void push_emplace_(ARGS&&...args)
      {
        static_cast<owner_type*>(this)->events_.push_back(make_handler_(std::forward<ARGS>(args)...));
      }

      void connect_(listener_type& l)
      {
        listeners_.push_back(&l);
      }

      void disconnect_(listener_type& l)
      {
        bool found = false;
        size_t   p = 0;
        for(listener_type*it : listeners_)
        {
          if(it == &l)
          {
            found = true;
            break;
          }
          ++p;
        }
        if(found)
        {
          size_t last_p = listeners_.size() - 1;
          if(last_p !=  p)
          {
            std::swap(listeners_[p], listeners_[last_p]);
          }
          listeners_.resize(last_p);
        }
      }

    private:
      class event_handler_impl : public event_handler
      {
        public:
          template <class ... ARGS> event_handler_impl(event_dispatcher& owner, ARGS&&... args)
            : owner_(owner)
            , event_(std::forward<ARGS>(args)...)
          {
          }

          virtual void process() override
          {
            listeners_type listeners = owner_.listeners_;
            for( listener_type* l : listeners)
            {
              l->handle(event_);
            }
          }

          virtual void dispose() override
          {
            owner_.free_handler_.emplace_back((data_event_handler_impl_type*)this);
            this->~event_handler_impl();
          }

        private:
          event_dispatcher& owner_;
          event_type        event_;
      };

      template <class ... ARGS> auto make_handler_(ARGS&&... args)
      {
        event_handler_deleter::ptr_type               ret;
        std::unique_ptr<data_event_handler_impl_type> mem;
        if(free_handler_.empty())
        {
          mem.reset(new data_event_handler_impl_type);
        }
        else
        {
          auto& handler = free_handler_.back();
          mem = std::move(handler);
          free_handler_.pop_back();
        }
        ret.reset(new(mem.release()) event_handler_impl(*this,  std::forward<ARGS>(args)...));
        return ret;
      }      

      typedef typename std::aligned_storage<sizeof(event_handler_impl), alignof(event_handler_impl)>::type  data_event_handler_impl_type;

      std::vector<std::unique_ptr<data_event_handler_impl_type>> free_handler_;
      listeners_type listeners_;
  };

  template <class O, class E0, class ... Es> class event_dispatcher<O, E0, Es...> : public event_dispatcher<O, E0>, public event_dispatcher<O, Es...>
  {
  };

  template <class... Events> class dispatcher : public event_dispatcher<dispatcher<Events...>, Events...>
  {
    public:
      typedef dispatcher self_type;
      template <class, class ...> friend class event_dispatcher;

      dispatcher()
        : event_idx_(0)
      {
      }

      ~dispatcher()
      {
        events_.clear();
      }

      size_t dispatch(int count = -1)
      {
        size_t ret = 0;
        for(; count && (event_idx_ < events_.size()) ; ++event_idx_)
        {
          events_[event_idx_]->process();
          events_[event_idx_].reset();
          ++ret;
          if(count > 0) --count;
        }
        if(event_idx_ == events_.size())
        {
          events_.resize(0);
          event_idx_ = 0;
        }
        return ret;
      }

      template <class Event> void push(Event && event)
      {
        event_dispatcher<self_type, Event>::push_(std::forward<Event>(event));
      }

      template <class Event> void push(const Event & event)
      {
        event_dispatcher<self_type, Event>::push_(event);
      }

      template <class Event, class ... ARGS> void push_emplace(ARGS && ... args)
      {
        event_dispatcher<self_type, Event>::push_emplace_(std::forward<ARGS>(args)...);
      }

      template <class Event> void connect(listener<Event> & l)
      {
        event_dispatcher<self_type, Event>::connect_(l);
      }
      template <class Event> void disconnect(listener<Event> & l)
      {
        event_dispatcher<self_type, Event>::disconnect_(l);
      }

    private:
      std::vector<event_handler_deleter::ptr_type> events_;
      size_t event_idx_;
  };
}

#endif
