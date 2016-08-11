#define BOOST_TEST_DYN_LINK
#define BOOST_AUTO_TEST_MAIN
#include <boost/test/auto_unit_test.hpp>
#include <boost/test/unit_test_suite.hpp>
#include <boost/test/unit_test.hpp>

#include <entity_system/segment.hpp>

#include <set>
#include <boost/mpl/list.hpp>

namespace
{
  uint32_t counter_data = 0;
  struct data
  {
    data(uint32_t p)
      : value(p)
    {
      ++counter_data;
    }

    ~data()
    {
      --counter_data;
    }

    uint32_t value;
  };
}

BOOST_TEST_DONT_PRINT_LOG_VALUE(data*);
BOOST_TEST_DONT_PRINT_LOG_VALUE(nullptr_t);

typedef boost::mpl::list<entity_system::segment<data, 8>, entity_system::segment<data, 16>, entity_system::segment<data, 32>, entity_system::segment<data, 64>> list_segment_type;
typedef boost::mpl::list<entity_system::dynamic_segment<data, 8>, entity_system::dynamic_segment<data, 16>, entity_system::dynamic_segment<data, 32>, entity_system::dynamic_segment<data, 64>> list_dynamic_segment_type;

BOOST_AUTO_TEST_CASE_TEMPLATE(segment, segment_type, list_segment_type)
{
  counter_data = 0;
  segment_type segment;

  // alloc
  std::vector<data*>   datas;
  for(uint32_t i = 0 ; i < segment_type::size ; ++i)
  {
    auto ret = segment.acquire(i);
    datas.push_back(ret.first);

    BOOST_CHECK_EQUAL(ret.second, i + 1);
    BOOST_CHECK_EQUAL(counter_data, i + 1);
  }
  BOOST_CHECK_EQUAL(std::set<data*>(datas.begin(), datas.end()).size(), segment_type::size);
  BOOST_CHECK_EQUAL(counter_data, segment_type::size);

  // limited alloc
  {
    auto null = segment.acquire(5);
    BOOST_CHECK_EQUAL(null.first,  nullptr);
    BOOST_CHECK_EQUAL(null.second, 0);
  }

  // align
  for(uint32_t i = 1 ; i < segment_type::size ; ++i)
  {
    size_t d = (uint8_t*)datas[i]  - (uint8_t*)datas[i-1];
    BOOST_CHECK_EQUAL(d, sizeof(data));
  }

  // iterate
  size_t i = 0;
  for(data*d : segment)
  {
    BOOST_CHECK_EQUAL(datas[i], d);
    ++i;
  }
  BOOST_CHECK_EQUAL(i, segment_type::size);

  // release one element
  segment.release(2);
  BOOST_CHECK_EQUAL(counter_data, segment_type::size - 1);

  // iterate
  i = 0;
  for(data*d : segment)
  {
    if(i == 1) i = 2;
    BOOST_CHECK_EQUAL(datas[i], d);
    ++i;
  }
  BOOST_CHECK_EQUAL(i, segment_type::size);

  // alloc
  {
    auto elt = segment.acquire(66);
    BOOST_CHECK_EQUAL(elt.first,  datas[1]);
    BOOST_CHECK_EQUAL(elt.second, 2);
    BOOST_CHECK_EQUAL(counter_data, segment_type::size);
  }

  // limited alloc
  {
    auto null = segment.acquire(5);
    BOOST_CHECK_EQUAL(null.first,  nullptr);
    BOOST_CHECK_EQUAL(null.second, 0);
  }

  // full desalloc
  segment.clear();
  BOOST_CHECK_EQUAL(counter_data, 0u);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(dynamic_segment, segment_type, list_dynamic_segment_type)
{
  counter_data = 0;
  segment_type segment;

  // alloc
  std::vector<data*>   datas;
  for(uint32_t i = 0 ; i < segment_type::size + 2; ++i)
  {
    auto ret = segment.acquire(i);
    datas.push_back(ret.first);

    BOOST_CHECK_EQUAL(ret.second.seg_id, i%segment_type::size + 1);
    BOOST_CHECK_EQUAL(ret.second.seg_nb, i/segment_type::size);
    BOOST_CHECK_EQUAL(counter_data, i + 1);
  }
  BOOST_CHECK_EQUAL(std::set<data*>(datas.begin(), datas.end()).size(), segment_type::size + 2);
  BOOST_CHECK_EQUAL(counter_data, segment_type::size + 2);

  // align
  for(uint32_t i = 1 ; i < segment_type::size ; ++i)
  {
    size_t d = (uint8_t*)datas[i]  - (uint8_t*)datas[i-1];
    BOOST_CHECK_EQUAL(d, sizeof(data));
  }
  BOOST_CHECK_EQUAL((size_t)((uint8_t*)datas[segment_type::size+1]  - (uint8_t*)datas[segment_type::size]), sizeof(data));

  // iterate
  size_t i = 0;
  for(data*d : segment)
  {
    BOOST_CHECK_EQUAL(datas[i], d);
    ++i;
  }
  BOOST_CHECK_EQUAL(i, segment_type::size + 2);

  // release elements
  static auto p1 = typename segment_type::id_type(2);
  static auto p2 = typename segment_type::id_type(1, 1);

  segment.release(p1);
  BOOST_CHECK_EQUAL(counter_data, segment_type::size + 1);
  segment.release(p2);
  BOOST_CHECK_EQUAL(counter_data, segment_type::size);

  // iterate
  i = 0;
  for(data*d : segment)
  {
    if(i == 1) i = 2;
    if(i == segment_type::size) i = segment_type::size + 1;
    BOOST_CHECK_EQUAL(datas[i], d);
    ++i;
  }
  BOOST_CHECK_EQUAL(i, segment_type::size + 2);

  // alloc
  {
    auto elt = segment.acquire(66);
    BOOST_CHECK_EQUAL(elt.first,  datas[1]);
    BOOST_CHECK_EQUAL(elt.second, p1);
    BOOST_CHECK_EQUAL(counter_data, segment_type::size + 1);
  }

  // alloc
  {
    auto elt = segment.acquire(66);
    BOOST_CHECK_EQUAL(elt.first,  datas[segment_type::size]);
    BOOST_CHECK_EQUAL(elt.second, p2);
    BOOST_CHECK_EQUAL(counter_data, segment_type::size + 2);
  }

  // full desalloc
  segment.clear();
  BOOST_CHECK_EQUAL(counter_data, 0u);
}
