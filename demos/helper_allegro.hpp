#ifndef DEMOS_HELPER_ALLEGRO_HPP
# define DEMOS_HELPER_ALLEGRO_HPP

#include <allegro5/allegro.h>

template <class R> class Handler
{
   private:
     R* data_;

   public:
     Handler(const Handler&) = delete;
     Handler& operator =(const Handler&) = delete;

     Handler()
       : data_(nullptr)
     {
     }

     Handler(R*data)
       : data_(data)
     {
     }

     Handler(Handler && handler)
       : data_(handler.data_)
     {
       handler.data_ = nullptr;
     }

     ~Handler()
     {
       reset();
     }

     operator R*() {return data_;}

     Handler& operator=(R* data)
     {
       reset();
       data_ = data;
       return *this;
     }

     void reset()
     {
       if(data_)
       {
         destroy_(data_);
         data_ = nullptr;
       }
     }

   private:
     void destroy_(R*);
};

template <> inline void Handler<ALLEGRO_TIMER>::destroy_(ALLEGRO_TIMER* timer)
{
   al_destroy_timer(timer);
}

template <> inline void Handler<ALLEGRO_EVENT_QUEUE>::destroy_(ALLEGRO_EVENT_QUEUE* queue)
{
   al_destroy_event_queue(queue);
}

template <> inline void Handler<ALLEGRO_BITMAP>::destroy_(ALLEGRO_BITMAP* bitmap)
{
   al_destroy_bitmap(bitmap);
}

template <> inline void Handler<ALLEGRO_DISPLAY>::destroy_(ALLEGRO_DISPLAY* display)
{
   al_destroy_display(display);
}

#endif
