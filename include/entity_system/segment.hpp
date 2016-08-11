#ifndef ENTITY_SYSTEM_SEGMENT_HPP
# define ENTITY_SYSTEM_SEGMENT_HPP

# include <entity_system/forwards.hpp>

# include <array>
# include <vector>
# include <type_traits>
# include <utility>
# include <memory>

namespace entity_system
{
  namespace detail
  {
    template <class T> struct segment_opt_tmpl
    {
        typedef T        flag_type;
        typedef uint8_t  pos_type;

        static constexpr flag_type all()
        {
          return (flag_type)-1;
        }

        static constexpr flag_type none()
        {
          return (flag_type)0;
        }

        static flag_type pos_2_mask(pos_type pos)
        {
          return (flag_type)1 << (pos - 1);
        }

        static pos_type find_first_bit(flag_type num)
        {
          pos_type ret = 0;
          static uint8_t nibblebits[] = {0, 1, 2, 1, 3, 1, 2, 1, 4, 1, 2, 1, 3, 1, 2, 1};
          for(;num != 0;ret += 4)
          {
            uint8_t t = num & 0x0f;
            if(t)
            {
              ret += nibblebits[t];
              break;
            }
            num >>= 4;
          }
          return ret;
        }

        static flag_type get_bit(flag_type mask)
        {
          static flag_type hitbits[] = {0, 1, 2, 1, 4, 1, 2, 1, 8, 1, 2, 1, 4, 1, 2, 1};
          for(char i = 0 ;mask; ++i)
          {
            unsigned char t = mask & 0x0f;
            if(t)
            {
              return hitbits[t] << 4*i;
            }
            mask >>= 4;
          }
          return 0;
        }

        static size_t count_bit(flag_type mask)
        {
          static uint8_t nibblebits[] = {0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4};
          std::size_t ret = 0;
          while(mask != 0)
          {
            uint8_t t = mask & 0x0f;
            ret += nibblebits[t];
            mask >>= 4;
          }
          return ret;
        }
    };

    template <size_t> struct segment_opt;
    template <> struct segment_opt<8> : public segment_opt_tmpl<uint8_t> {};
    template <> struct segment_opt<16> : public segment_opt_tmpl<uint16_t> {};
    template <> struct segment_opt<32> : public segment_opt_tmpl<uint32_t> {};
    template <> struct segment_opt<64> : public segment_opt_tmpl<uint64_t> {};
  }

  template <class Seg> class segment_iterator
  {
    public:
      typedef Seg segment_type;
      typedef typename segment_type::id_type id_type;

      segment_iterator(segment_type& segment, id_type pos)
        : segment_(segment)
        , pos_(segment_.next(pos))
      {
      }

      segment_iterator& operator++()
      {
        pos_ = segment_.next(pos_);
        return *this;
      }

      segment_iterator operator++(int)
      {
        return segment_iterator(segment_, segment_.next(pos_));
      }

      bool operator==(const segment_iterator& other) const
      {
        return other.pos_ == pos_;
      }

      bool operator!=(const segment_iterator& other) const
      {
        return ! (*this == other);
      }

      auto operator*()
      {
        return segment_.get(pos_);
      }

      auto operator*() const
      {
        return segment_.get(pos_);
      }

    private:
      segment_type& segment_;
      id_type       pos_;
  };

  template <class T, size_t S> class segment
  {
    public:
      typedef T                                                                 type;
      typedef detail::segment_opt<S>                                            opt_type;
      typedef typename opt_type::pos_type                                       id_type;
      typedef typename opt_type::flag_type                                      flag_type;
      typedef typename std::aligned_storage<sizeof(type), alignof(type)>::type  data_type;
      typedef std::array<data_type, S>                                          array_data_type;
      typedef segment_iterator<segment>                                         iterator;
      typedef segment_iterator<const segment>                                   const_iterator;

      static const size_t size;

      segment()
        : flag_(opt_type::all())
      {
      }

      ~segment()
      {
        clear();
      }

      static constexpr id_type max_pos()
      {
        return S + 1;
      }

      void clear()
      {
        for(id_type id = 1; id < max_pos(); ++id)
        {
          if(has(id))
          {
            release(id);
          }
        }
      }

      template <class ... ARGS> std::pair<type*, id_type> acquire(ARGS && ... args)
      {
        id_type pos = opt_type::find_first_bit(flag_);
        type*   val = nullptr;
        if(pos)
        {
          void* data = &data_[pos - 1];
          flag_      = flag_ & ~opt_type::pos_2_mask(pos);
          new(data) type(std::forward<ARGS>(args)...);
          val        = (type*)data;
        }
        return std::make_pair(val, pos);
      }

      void release(id_type id)
      {
        type* val = get(id);
        if(val)
        {
          val->~type();
          flag_ = flag_ | opt_type::pos_2_mask(id);
        }
      }

      bool full() const
      {
        return opt_type::count_bit(flag_) == 0;
      }

      bool has(id_type id) const
      {
        return (id > 0) && (id <= S) && (opt_type::pos_2_mask(id) & ~flag_);
      }

      type* get(id_type id)
      {
        return (has(id) ? (type*)&data_[id - 1] : nullptr);
      }

      const type* get(id_type id) const
      {
        return (has(id) ? (const type*)&data_[id - 1] : nullptr);
      }

      iterator begin()
      {
        return iterator(*this, 0);
      }

      iterator end()
      {
        return iterator(*this, max_pos());
      }

      const_iterator begin() const
      {
        return cbegin();
      }

      const_iterator end() const
      {
        return cend();
      }

      const_iterator cbegin() const
      {
        return const_iterator(*this, 0);
      }

      const_iterator cend() const
      {
        return iterator(*this, max_pos());
      }

      id_type next(id_type pos) const
      {
        if(pos < max_pos())
        {
          ++pos;
          for(;pos < max_pos(); ++pos)
          {
            if(has(pos))
            {
              break;
            }
          }
        }
        return pos;
      }

    private:
      flag_type       flag_;
      array_data_type data_;
  };
  template <class T, size_t S> const size_t segment<T, S>::size = S;

  template <class T, size_t S> class dynamic_segment
  {
    public:
      typedef T                type;
      typedef segment<type, S> segment_type;
      struct id_type
      {
        id_type(uint32_t val)
          : int_value(val)
        {
        }

        id_type(uint32_t id, uint32_t nb)
          : seg_id(id)
          , seg_nb(nb)
        {
        }

        operator uint32_t() const
        {
          return int_value;
        }

        union
        {
          uint32_t int_value;
          struct
          {
            uint32_t seg_id : 8;
            uint32_t seg_nb : 24;
          };
        };
      };
      typedef segment_iterator<dynamic_segment>       iterator;
      typedef segment_iterator<const dynamic_segment> const_iterator;

      static const size_t size;

      dynamic_segment()
      {
      }

      ~dynamic_segment()
      {
        clear();
      }

      void clear()
      {
        segments_.clear();
      }

      template <class ... ARGS> std::pair<type*, id_type> acquire(ARGS && ... args)
      {
        segment_type* segment = nullptr;
        id_type id(0);
        for(auto& seg : segments_)
        {
          if(!seg->full())
          {
            segment = seg.get();
            break;
          }
          ++id.seg_nb;
        }

        if(!segment)
        {
          std::unique_ptr<segment_type> seg = std::make_unique<segment_type>();
          segment = seg.get();
          segments_.push_back(std::move(seg));
        }

        type* data;
        typename segment_type::id_type seg_id;
        std::tie(data, seg_id) = segment->acquire(std::forward<ARGS>(args)...);
        id.seg_id = seg_id;

        return std::make_pair(data, id);
      }

      void release(id_type id)
      {
        segments_[id.seg_nb]->release(id.seg_id);
      }

      bool has(id_type id) const
      {
        return (id.seg_nb < segments_.size()) && (segments_[id.seg_nb]->has(id.seg_id));
      }

      type* get(id_type id)
      {
        return (has(id) ? segments_[id.seg_nb]->get(id.seg_id) : nullptr);
      }

      const type* get(id_type id) const
      {
        return (has(id) ? segments_[id.seg_nb]->get(id.seg_id) : nullptr);
      }

      iterator begin()
      {
        return iterator(*this, begin_pos_());
      }

      iterator end()
      {
        return iterator(*this, end_pos_());
      }

      const_iterator begin() const
      {
        return cbegin();
      }

      const_iterator end() const
      {
        return cend();
      }

      const_iterator cbegin() const
      {
        return const_iterator(*this, begin_pos_());
      }

      const_iterator cend() const
      {
        return iterator(*this, end_pos_());
      }

      id_type next(id_type pos) const
      {
        id_type end = end_pos_();
        if(pos != end)
        {
          do
          {
            pos.seg_id = segments_[pos.seg_nb]->next(pos.seg_id);
            if(pos.seg_id == segment_type::max_pos())
            {
              pos.seg_id = 0;
              if(segments_.size() == pos.seg_nb + 1u)
              {
                pos = end;
                break;
              }
              else
              {
                ++pos.seg_nb;
              }
            }
          } while(!has(pos));
        }
        return pos;
      }

    protected:
      typedef std::vector<std::unique_ptr<segment_type>> segments_type;

      id_type end_pos_() const
      {
        return id_type(S + 1, (segments_.empty() ? 0 : segments_.size() - 1));
      }

      id_type begin_pos_() const
      {
        return (segments_.empty() ? end_pos_() : id_type(0));
      }

    private:
      segments_type segments_;
  };
  template <class T, size_t S> const size_t dynamic_segment<T, S>::size = S;
}

#endif
