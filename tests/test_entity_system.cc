#define BOOST_TEST_DYN_LINK
#define BOOST_AUTO_TEST_MAIN
#include <boost/test/auto_unit_test.hpp>
#include <boost/test/unit_test_suite.hpp>
#include <boost/test/unit_test.hpp>

#include <set>
#include <entity_system/entity_system.hpp>

namespace
{
  class position
  {
    public:
      position(uint16_t x, uint16_t y)
        : x(x)
        , y(y)
      {
      }

      ~position()
      {
      }

      uint16_t x;
      uint16_t y;
  };

    class life
    {
      public:
       life(uint16_t init)
        : init(init)
       {
       }

       uint16_t init;
    };

  class e1 {};
  class e2 {};

  typedef entity_system::world<std::tuple<e1, e2>, std::tuple<position, life>> world_type;
}

BOOST_AUTO_TEST_CASE( entity_system_01 )
{
  world_type world;

  auto& em    = world.get_entity_manager();
  auto entity = em.new_entity();

  BOOST_CHECK(entity->get_component<position>() == nullptr);
  BOOST_CHECK(entity->get_component<life>()     == nullptr);

  position* p = entity->new_component<position>(5, 8);
  life*     l = entity->new_component<life>(15);

  BOOST_REQUIRE(p != nullptr);
  BOOST_REQUIRE(l != nullptr);

  BOOST_CHECK_EQUAL(p->x, 5);
  BOOST_CHECK_EQUAL(p->y, 8);

  BOOST_CHECK_EQUAL(l->init, 15);

  BOOST_CHECK(entity->get_component<position>() == p);
  BOOST_CHECK(entity->get_component<life>()     == l);

  entity->delete_component<life>();
  BOOST_CHECK(entity->get_component<life>()     == nullptr);

  entity->delete_all_components();
  BOOST_CHECK(entity->get_component<position>() == nullptr);
}

BOOST_AUTO_TEST_CASE( entity_system_02 )
{
  world_type world;

  auto& em     = world.get_entity_manager();
  auto entity1 = em.new_entity();
  auto entity2 = em.new_entity();

  entity1->new_component<position>(5, 8);
  entity1->new_component<life>(15);

  entity2->new_component<position>(5, 8);

  {
    size_t len = 0;
    std::set<world_type::entity_type*> es {entity1, entity2};
    em.for_entities_with<position>( [&]( auto& e)
    {
      es.erase(&e);
      ++len;
    });
    BOOST_CHECK_EQUAL(len, 2u);
    BOOST_CHECK(es.empty());
  }

  {
    size_t len = 0;
    std::set<world_type::entity_type*> es {entity1};
    em.for_entities_with<life>( [&]( auto& e)
    {
      es.erase(&e);
      ++len;
    });
    BOOST_CHECK_EQUAL(len, 1u);
    BOOST_CHECK(es.empty());
  }

  em.delete_entity(*entity2);

  {
    size_t len = 0;
    std::set<world_type::entity_type*> es {entity1};
    em.for_entities_with<position>( [&]( auto& e)
    {
      es.erase(&e);
      ++len;
    });
    BOOST_CHECK_EQUAL(len, 1u);
    BOOST_CHECK(es.empty());
  }

  {
    size_t len = 0;
    std::set<world_type::entity_type*> es {entity1};
    em.for_entities_with<life>( [&]( auto& e)
    {
      es.erase(&e);
      ++len;
    });
    BOOST_CHECK_EQUAL(len, 1u);
    BOOST_CHECK(es.empty());
  }

  em.delete_entity(*entity1);

  {
    size_t len = 0;
    em.for_entities_with<position>( [&]( auto&)
    {
      ++len;
    });
    BOOST_CHECK_EQUAL(len, 0u);
  }

  {
    size_t len = 0;
    em.for_entities_with<life>( [&]( auto&)
    {
      ++len;
    });
    BOOST_CHECK_EQUAL(len, 0u);
  }
}
