#include "helper_allegro.hpp"
#include <entity_system/entity_system.hpp>
#include <iostream>

#include<allegro5/allegro_primitives.h>

//
// component
class position
{
  public:
    position(int16_t x, int16_t y)
      : x(x)
      , y(y)
    {
    }

    friend std::ostream& operator << (std::ostream& stream , const position &p);

    int16_t x;
    int16_t y;
};

inline std::ostream& operator << (std::ostream& stream , const position &p)
{
  stream << "(" << p.x << ", " << p.y << ")";
  return stream;
}

class velocity
{
  public:
    velocity(int16_t x, int16_t y)
      : x(x)
      , y(y)
    {
    }

    bool operator ==(const velocity&v) const
    {
      return (x == v.x) && (y == v.y);
    }

    int16_t x;
    int16_t y;
};

class score
{
  public:
    score()
      : value(0)
    {
    }

    uint16_t value;
};

class position_history
{
  public:
    position_history()
    {
    }

    std::vector<position> positions;
};

class target {};

//
// event
class keyboard
{
  public:
    enum class direction_type
    {
      right,
      left
    };

    keyboard(direction_type d)
      : direction(d)
    {
    }

    direction_type direction;
};

class refresh_view {};
class apply_move {};
class stop_game {};

// entity_system
typedef std::tuple<keyboard, apply_move, refresh_view, stop_game> events_type;
typedef std::tuple<position, velocity, score, target, position_history> components_type;

typedef entity_system::world<events_type, components_type> world_type;
typedef world_type::entity_type                            entity_type;
typedef world_type::entity_manager_type                    entity_manager_type;
typedef world_type::system_manager_type                    system_manager_type;
typedef world_type::dispatcher_type                        dispatcher_type;

// system
class sys_move  : public entity_system::system
                , public entity_system::listener<keyboard>
                , public entity_system::listener<apply_move>
{
  public:
    sys_move(world_type& world)
      : world_(world)
      , dispatcher_(world.get_system_manager().get_dispatcher())
      , entity_manager_(world.get_entity_manager())
    {
      dispatcher_.connect<keyboard>(*this);
      dispatcher_.connect<apply_move>(*this);

      for(int i = 0 ; i < 300 ; ++i)
      {
        for(int j = 0 ; j < 150 ; ++j)
        {
          already_view_[i][j] = false;
        }
      }
    }

    virtual ~sys_move()
    {
      dispatcher_.disconnect<keyboard>(*this);
      dispatcher_.disconnect<apply_move>(*this);
    }

    virtual void handle(keyboard& event) override
    {
      entity_manager_.for_entities_with<velocity>( [&event](entity_type& player)
      {
        velocity* v = player.get_component<velocity>();
        assert(v);

        static const velocity all_velocities[] = {
          velocity(5,  0),
          velocity(0,  5),
          velocity(-5, 0),
          velocity(0, -5)
        };

        int id = 0;
        for(const velocity& ref_v : all_velocities)
        {
          if(ref_v == *v)
          {
            switch(event.direction)
            {
              case keyboard::direction_type::left:
                id = (id > 0 ? id - 1 : 3);
                break;

              case keyboard::direction_type::right:
                id = (id < 3 ? id + 1 : 0);
                break;
            }
            break;
          }
          ++id;
        }
        if(id == 4)
        {
          switch(event.direction)
          {
            case keyboard::direction_type::left:
              id = 2;
              break;

            case keyboard::direction_type::right:
              id = 0;
              break;
          }
        }

        *v = all_velocities[id];
      });
    }

    virtual void handle(apply_move&) override
    {
      entity_manager_.for_entities_with<position, velocity, position_history>( [&](entity_type& player)
      {
        velocity* v         = player.get_component<velocity>();
        position* p         = player.get_component<position>();
        position_history* h = player.get_component<position_history>();

        assert(v);
        assert(p);
        assert(h);

        if(v->x != 0 || v->y != 0)
        {
          int16_t x = p->x + v->x;
          int16_t y = p->y + v->y;

          if( !apply_(player, *p, x, y) )
          {
            this->stop_();
          }

          p->x = x;
          p->y = y;

          h->positions.push_back(*p);

          std::cout << "x:" << p->x << " y:" << p->y << std::endl;
        }
      });
    }

  protected:
    void stop_()
    {
      dispatcher_.push_emplace<stop_game>();
    }

    bool apply_(entity_type& player, position& p, int16_t &x, int16_t &y)
    {
      // increase point
      score* s = player.get_component<score>();
      if(s)
      {
        int howmany_target = 0;
        entity_manager_.for_entities_with<position, target>( [&](entity_type& target)
        {
          position* target_p = target.get_component<position>();
          bool result        = false;

          if(target_p->y == y)
          {
            if(p.x < x)
            {
              result = (p.x <= target_p->x) && (target_p->x <= x);
            }
            else if(p.x > x)
            {
              result = (p.x >= target_p->x) && (target_p->x >= x);
            }
          }
          else if(target_p->x == x)
          {
            if(p.y < y)
            {
              result = (p.y <= target_p->y) && (target_p->y <= y);
            }
            else if(p.y > y)
            {
              result = (p.y >= target_p->y) && (target_p->y >= y);
            }
          }

          if(result)
          {
            std::cout << "point !!" << std::endl;
            s->value += 1;
            this->entity_manager_.delete_entity(target);
          }
          else
          {
            ++howmany_target;
          }
        });

        if(!howmany_target)
        {
          return false;
        }
      }

      // out
      if(x < 0 || y < 0 || x >= 300 || y >= 150 )
      {
        x = std::min<int16_t>(std::max<int16_t>(x, 0), 299);
        y = std::min<int16_t>(std::max<int16_t>(y, 0), 149);
        return false;
      }

      // collision
      if(p.x == x && p.y != y)
      {
        for(int16_t i = std::min(y, p.y) ; i <= std::max(y, p.y) ; ++i)
        {
          if(already_view_[x][i])
          {
            y = i;
            return false;
          }
          already_view_[x][i] = true;
        }
      }

      if(p.y == y && p.x != x)
      {
        for(int16_t i = std::min(x, p.x) ; i <= std::max(x, p.x) ; ++i)
        {
          if(already_view_[i][y])
          {
            x = i;
            return false;
          }
          already_view_[i][y] = true;
        }
      }
      already_view_[x][y] = false;

      return true;
    }

  private:
    world_type&          world_;
    dispatcher_type&     dispatcher_;
    entity_manager_type& entity_manager_;
    bool                 already_view_[300][150];
};

class allegro_draw  : public entity_system::system
                    , public entity_system::listener<refresh_view>
{
  public:
    allegro_draw(world_type& world)
      : world_(world)
      , dispatcher_(world.get_system_manager().get_dispatcher())
      , entity_manager_(world.get_entity_manager())
    {
      dispatcher_.connect<refresh_view>(*this);
    }

    ~allegro_draw()
    {
      dispatcher_.disconnect<refresh_view>(*this);
    }

    virtual void handle(refresh_view&) override
    {
      al_clear_to_color(al_map_rgb(0, 0, 0));

      entity_manager_.for_entities_with<position_history>( [&](entity_type& player)
      {
        position_history* h = player.get_component<position_history>();

        for(size_t i = 1 ; i < h->positions.size() ; ++i)
        {
          const position& p1 = h->positions[i-1];
          const position& p2 = h->positions[i];
          al_draw_line(p1.x, p1.y, p2.x, p2.y, al_map_rgb(255, 255, 255), 1);
        }
      });

      entity_manager_.for_entities_with<position, target>( [&](entity_type& target)
      {
        position* p = target.get_component<position>();
        al_draw_pixel(p->x, p->y, al_map_rgb(255, 0, 0));
      });

      al_flip_display();
    }

  private:
    world_type&          world_;
    dispatcher_type&     dispatcher_;
    entity_manager_type& entity_manager_;
};

class application : public entity_system::listener<stop_game>
{
  public:
    application()
      : stop_(false)
      , trigger_refresh_view_(false)
      , entity_manager_(world_.get_entity_manager())
      , system_manager_(world_.get_system_manager())
      , dispatcher_(system_manager_.get_dispatcher())
    {
      // build the ... world
      entity_type* player = entity_manager_.new_entity();
      position* init_position = player->new_component<position>(150, 75);
      player->new_component<velocity>(0,   0);
      player->new_component<score>();
      player->new_component<position_history>()->positions.push_back(*init_position);

      for(int i = 0 ; i < 4 ; ++i)
      {
        entity_type* t = entity_manager_.new_entity();
        t->new_component<position>(75*(i+1), 125);
        t->new_component<target>();
      }

      dispatcher_.connect<stop_game>(*this);
    }

    ~application()
    {
      dispatcher_.disconnect<stop_game>(*this);
    }

    int run()
    {
      if(!init_())
      {
        return -1;
      }

      system_manager_.add_system(new sys_move(world_));
      system_manager_.add_system(new allegro_draw(world_));

      al_clear_to_color(al_map_rgb(0, 0, 0));
      al_flip_display();
      al_start_timer(timer_);

      while(!stop_)
      {
        ALLEGRO_EVENT event;
        al_wait_for_event(queue_, &event);

        switch(event.type)
        {
          case ALLEGRO_EVENT_TIMER:
            onEvent_(event.timer);
            break;

          case ALLEGRO_EVENT_DISPLAY_CLOSE:
            dispatcher_.push_emplace<stop_game>();
            break;

          case ALLEGRO_EVENT_KEY_CHAR:
            onEvent_(event.keyboard);
            break;
        }

        process_();
      }

      return 0;
    }

    virtual void handle(stop_game&) override
    {
      entity_manager_.for_entities_with<score>( [](entity_type& player)
      {
        score* s = player.get_component<score>();
        assert(s);
        std::cout << "stop with " << s->value << std::endl;
      });
      stop_ = true;
    }

  protected:
    bool init_()
    {
      if(!al_init())
      {
        std::cerr << "Failed to initialize allegro" << std::endl;
        return false;
      }

      if(!al_install_keyboard())
      {
        std::cerr << "Failed to initialize allegro/keyboard" << std::endl;
        return false;
      }

      queue_   = al_create_event_queue();
      timer_   = al_create_timer(10.0 / 60.0 );
      display_ = al_create_display(300, 150);

      al_register_event_source(queue_, al_get_display_event_source(display_));
      al_register_event_source(queue_, al_get_timer_event_source(timer_));
      al_register_event_source(queue_, al_get_keyboard_event_source());

      return true;
    }

    void process_()
    {
      if(trigger_refresh_view_ && al_is_event_queue_empty(queue_))
      {
        trigger_refresh_view_ = false;
        dispatcher_.push_emplace<refresh_view>();
      }

      system_manager_.process();
    }

    void onEvent_(ALLEGRO_TIMER_EVENT&)
    {
      dispatcher_.push_emplace<apply_move>();
      trigger_refresh_view_ = true;
    }

    void onEvent_(ALLEGRO_KEYBOARD_EVENT& event)
    {
      switch(event.keycode)
      {
        case ALLEGRO_KEY_RIGHT:
          dispatcher_.push_emplace<keyboard>(keyboard::direction_type::right);
          break;
        case ALLEGRO_KEY_LEFT:
          dispatcher_.push_emplace<keyboard>(keyboard::direction_type::left);
          break;
        case ALLEGRO_KEY_ESCAPE:
          dispatcher_.push_emplace<stop_game>();
          break;
      }
    }

  private:
    bool stop_;
    bool trigger_refresh_view_;

    world_type                    world_;
    entity_manager_type& entity_manager_;
    system_manager_type& system_manager_;
    dispatcher_type&         dispatcher_;

    Handler<ALLEGRO_EVENT_QUEUE>  queue_;
    Handler<ALLEGRO_TIMER>        timer_;
    Handler<ALLEGRO_DISPLAY>    display_;
};

int main()
{
  application  app;
  return app.run();
}
