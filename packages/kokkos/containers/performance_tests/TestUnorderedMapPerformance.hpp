#ifndef KOKKOS_TEST_UNORDERED_MAP_PERFORMANCE_HPP
#define KOKKOS_TEST_UNORDERED_MAP_PERFORMANCE_HPP

#include <impl/Kokkos_Timer.hpp>

#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <sstream>


namespace Perf {

template <typename Device, bool Near>
struct UnorderedMapTest
{
  typedef Device device_type;
  typedef Kokkos::UnorderedMap<uint32_t, uint32_t, device_type> map_type;
  typedef typename map_type::histogram_type histogram_type;
  typedef uint32_t value_type;

  uint32_t capacity;
  uint32_t inserts;
  uint32_t collisions;
  double   seconds;
  map_type map;
  histogram_type histogram;

  UnorderedMapTest( uint32_t arg_capacity, uint32_t arg_inserts, uint32_t arg_collisions)
    : capacity(arg_capacity)
    , inserts(arg_inserts)
    , collisions(arg_collisions)
    , seconds(0)
    , map(capacity)
    , histogram(map.get_histogram())
  {
    Kokkos::Impl::Timer wall_clock ;
    wall_clock.reset();

    uint32_t failed_count = 0;
    int loop_count = 0;
    do {
      ++loop_count;

      failed_count = 0;
      Kokkos::parallel_reduce(inserts, *this, failed_count);

      if (failed_count > 0u) {
        const uint32_t new_capacity = map.capacity() + ((map.capacity()*3ull)/20u) + failed_count/collisions ;
        map.rehash( new_capacity );
      }
    } while (failed_count > 0u);

    seconds = wall_clock.seconds();

    switch (loop_count)
    {
    case 1u: std::cout << " \033[0;32m" << loop_count << "\033[0m "; break;
    case 2u: std::cout << " \033[1;31m" << loop_count << "\033[0m "; break;
    default: std::cout << " \033[0;31m" << loop_count << "\033[0m "; break;
    }
    std::cout << std::setprecision(2) << std::fixed << std::setw(5) << (1e9*(seconds/(inserts))) << " " << std::flush;

    //histogram.calculate();
    //Device::fence();
  }

  void print(std::ostream & metrics_out, std::ostream & length_out, std::ostream & distance_out, std::ostream & block_distance_out)
  {
    metrics_out << map.capacity() << " , ";
    metrics_out << inserts/collisions << " , ";
    metrics_out << (100.0 * inserts/collisions) / map.capacity() << " , ";
    metrics_out << inserts << " , ";
    metrics_out << (map.failed_insert() ? "true" : "false") << " , ";
    metrics_out << collisions << " , ";
    metrics_out << 1e9*(seconds/inserts) << " , ";
    metrics_out << seconds << std::endl;

    length_out << map.capacity() << " , ";
    length_out << ((100.0 *inserts/collisions) / map.capacity()) << " , ";
    length_out << collisions << " , ";
    histogram.print_length(length_out);

    distance_out << map.capacity() << " , ";
    distance_out << ((100.0 *inserts/collisions) / map.capacity()) << " , ";
    distance_out << collisions << " , ";
    histogram.print_distance(distance_out);

    block_distance_out << map.capacity() << " , ";
    block_distance_out << ((100.0 *inserts/collisions) / map.capacity()) << " , ";
    block_distance_out << collisions << " , ";
    histogram.print_block_distance(block_distance_out);
  }


  KOKKOS_INLINE_FUNCTION
  void init( value_type & failed_count ) const { failed_count = 0; }

  KOKKOS_INLINE_FUNCTION
  void join( volatile value_type & failed_count, const volatile value_type & count ) const
  { failed_count += count; }

  KOKKOS_INLINE_FUNCTION
  void operator()(uint32_t i, value_type & failed_count) const
  {
    const uint32_t key = Near ? i/collisions : i%(inserts/collisions);
    if (map.insert(key,i).failed()) ++failed_count;
  }

};

#define KOKKOS_COLLECT_UNORDERED_MAP_METRICS

template <typename Device, bool Near>
void run_performance_tests(std::string const & base_file_name)
{
#if defined(KOKKOS_COLLECT_UNORDERED_MAP_METRICS)
  std::string metrics_file_name = base_file_name + std::string("-metrics.csv");
  std::string length_file_name = base_file_name  + std::string("-length.csv");
  std::string distance_file_name = base_file_name + std::string("-distance.csv");
  std::string block_distance_file_name = base_file_name + std::string("-block_distance.csv");

  std::ofstream metrics_out( metrics_file_name.c_str(), std::ofstream::out );
  std::ofstream length_out( length_file_name.c_str(), std::ofstream::out );
  std::ofstream distance_out( distance_file_name.c_str(), std::ofstream::out );
  std::ofstream block_distance_out( block_distance_file_name.c_str(), std::ofstream::out );


  const double test_ratios[] = {
     0.50
   , 0.75
   , 0.775
   , 0.80
   , 0.825
   , 0.85
   , 0.875
   , 0.90
   , 0.925
   , 1.00
   , 1.10
  };
  const int num_ratios = sizeof(test_ratios) / sizeof(double);

  // set up file headers
  metrics_out << "Capacity , Unique , Percent Full , Attempted Inserts , Failed Inserts , Collision Ratio , Nanoseconds/Inserts, Seconds" << std::endl;
  length_out << "Capacity , Percent Full , ";
  distance_out << "Capacity , Percent Full , ";
  block_distance_out << "Capacity , Percent Full , ";

  for (int i=0; i<100; ++i) {
    length_out << i << " , ";
    distance_out << i << " , ";
    block_distance_out << i << " , ";
  }

  length_out << "\b\b\b   " << std::endl;
  distance_out << "\b\b\b   " << std::endl;
  block_distance_out << "\b\b\b   " << std::endl;

  Kokkos::Impl::Timer wall_clock ;
  for (uint32_t collisions = 1;  collisions <= 64u; collisions = collisions << 1) {
    wall_clock.reset();
    std::cout << "Collisions: " << collisions << std::endl;
    for (int i = 0; i < num_ratios; ++i) {
      std::cout << std::setprecision(1) << std::fixed << std::setw(5) << (100.0*test_ratios[i]) << "%  " << std::flush;
      for (uint32_t capacity = 1<<14; capacity < 1<<26; capacity = capacity << 1) {
        uint32_t inserts = static_cast<uint32_t>(test_ratios[i]*(capacity));
        std::cout << capacity << std::flush;
        UnorderedMapTest<Device, Near> test(capacity, inserts*collisions, collisions);
        Device::fence();
        //test.print(metrics_out, length_out, distance_out, block_distance_out);
      }
      std::cout << std::endl;
    }
    std::cout << "  " << wall_clock.seconds() << " secs" << std::endl;
  }
  metrics_out.close();
  length_out.close();
  distance_out.close();
  block_distance_out.close();
#else
  (void)base_file_name;
  std::cout << "skipping test" << std::endl;
#endif
}


} // namespace Perf

#endif //KOKKOS_TEST_UNORDERED_MAP_PERFORMANCE_HPP
