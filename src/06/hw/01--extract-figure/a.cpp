#include <algorithm>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <ranges>

#include "../../utils/opencv.hpp"

int main(int argc, char *argv[], char *envp[])
{

  auto arr = std::vector<int>{1, 2, 3, 4, 5};
  auto fn2x = [](int x) { return x * 2; };
  auto pred = [](int x) { return x % 3 == 0; };
  auto arr2x = arr                           //
               | std::views::transform(fn2x) //
               | std::views::filter(pred);

  for (const auto &x : arr2x)
  {
    std::cout << x << std::endl;
  }

  return 0;
}
