#define BOOST_TEST_DYN_LINK
#define BOOST_AUTO_TEST_MAIN
#include <boost/test/auto_unit_test.hpp>
#include <boost/test/unit_test_suite.hpp>
#include <boost/test/unit_test.hpp>

#include <entity_system/event_dispatcher.hpp>

namespace
{
  struct event1
  {
    std::string data;
    uint32_t    id;

    bool operator ==(const event1& e) const
    {
      return (e.data == data) && (e.id == id);
    }
  };

  struct event2
  {
    event2() {}
    event2(const std::string&d, uint32_t i) : data(d), id(i) {}

    std::string data;
    uint32_t    id;

    bool operator ==(const event2& e) const
    {
      return (e.data == data) && (e.id == id);
    }
  };

  typedef entity_system::dispatcher<event1, event2> dispatcher_type;

  class handler_ev1 : public entity_system::listener<event1>
  {
    public:
      handler_ev1() : count_(0) {}

      virtual void handle(event1& e) override
      {
        ++count_;
        BOOST_CHECK_EQUAL(e, ref_);
      }

      uint32_t count_;
      event1   ref_;
  };

  class handler_reentrant_ev1 : public entity_system::listener<event1>
  {
    public:
      handler_reentrant_ev1(dispatcher_type& dispatcher)
        : dispatcher_(dispatcher)
      {
      }

      virtual void handle(event1& e) override
      {
        dispatcher_.push_emplace<event2>(e.data, e.id);
      }

      dispatcher_type& dispatcher_;
  };

  class handler_ev2 : public entity_system::listener<event2>
  {
    public:
      handler_ev2() : count_(0) {}

      virtual void handle(event2& e) override
      {
        ++count_;
        BOOST_CHECK_EQUAL(e, ref_);
      }

      uint32_t count_;
      event2   ref_;
  };

  class handler_ev1_ev2 : public entity_system::listener<event1>, public entity_system::listener<event2>
  {
    public:
      handler_ev1_ev2() : count1_(0), count2_(0) {}

      virtual void handle(event1& e) override
      {
        ++count1_;
        BOOST_CHECK_EQUAL(e, ref1_);
      }

      virtual void handle(event2& e) override
      {
        ++count2_;
        BOOST_CHECK_EQUAL(e, ref2_);
      }

      uint32_t count1_;
      uint32_t count2_;
      event1   ref1_;
      event2   ref2_;
  };
}

BOOST_TEST_DONT_PRINT_LOG_VALUE(event1);
BOOST_TEST_DONT_PRINT_LOG_VALUE(event2);

BOOST_AUTO_TEST_CASE( event_dispatcher_01 )
{
  dispatcher_type dispatcher;

  handler_ev1     h1;
  handler_ev2     h2;
  handler_ev1_ev2 h3;

  dispatcher.connect(h1);
  dispatcher.connect(h2);
  dispatcher.connect<event1>(h3);
  dispatcher.connect<event2>(h3);

  event1 e1{"event1", 5};
  event2 e2{"event2", 10};

  h1.ref_ = e1;
  h2.ref_ = e2;
  h3.ref1_ = e1;
  h3.ref2_ = e2;

  dispatcher.push<event1>(e1);

  BOOST_CHECK_EQUAL(dispatcher.dispatch(), 1u);
  BOOST_CHECK_EQUAL(h1.count_ , 1u);
  BOOST_CHECK_EQUAL(h2.count_ , 0u);
  BOOST_CHECK_EQUAL(h3.count1_, 1u);
  BOOST_CHECK_EQUAL(h3.count2_, 0u);

  dispatcher.push<event2>(e2);

  BOOST_CHECK_EQUAL(dispatcher.dispatch(), 1u);
  BOOST_CHECK_EQUAL(h1.count_ , 1u);
  BOOST_CHECK_EQUAL(h2.count_ , 1u);
  BOOST_CHECK_EQUAL(h3.count1_, 1u);
  BOOST_CHECK_EQUAL(h3.count2_, 1u);

  dispatcher.push<event1>(e1);
  dispatcher.push<event2>(e2);

  BOOST_CHECK_EQUAL(dispatcher.dispatch(), 2u);
  BOOST_CHECK_EQUAL(h1.count_ , 2u);
  BOOST_CHECK_EQUAL(h2.count_ , 2u);
  BOOST_CHECK_EQUAL(h3.count1_, 2u);
  BOOST_CHECK_EQUAL(h3.count2_, 2u);

  dispatcher.disconnect<event1>(h3);
  dispatcher.push<event1>(e1);

  BOOST_CHECK_EQUAL(dispatcher.dispatch(), 1u);
  BOOST_CHECK_EQUAL(h1.count_ , 3u);
  BOOST_CHECK_EQUAL(h2.count_ , 2u);
  BOOST_CHECK_EQUAL(h3.count1_, 2u);
  BOOST_CHECK_EQUAL(h3.count2_, 2u);

  dispatcher.push<event2>(e2);

  BOOST_CHECK_EQUAL(dispatcher.dispatch(), 1u);
  BOOST_CHECK_EQUAL(h1.count_ , 3u);
  BOOST_CHECK_EQUAL(h2.count_ , 3u);
  BOOST_CHECK_EQUAL(h3.count1_, 2u);
  BOOST_CHECK_EQUAL(h3.count2_, 3u);
}

BOOST_AUTO_TEST_CASE( event_dispatcher_02 )
{
  dispatcher_type dispatcher;

  handler_reentrant_ev1 h1(dispatcher);
  handler_ev2           h2;

  dispatcher.connect(h1);
  dispatcher.connect(h2);

  event1 e1{"event1", 5};
  event2 e2{"event1", 5};

  h2.ref_ = e2;

  dispatcher.push<event1>(e1);

  BOOST_CHECK_EQUAL(dispatcher.dispatch(), 2u);
  BOOST_CHECK_EQUAL(h2.count_ , 1u);
}
