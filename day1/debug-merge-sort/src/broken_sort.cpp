#include <iostream>
#include <vector>

std::vector<int> merge(std::vector<int> left,
                       std::vector<int> right)
{
    std::vector<int> result;
    result.reserve(left.size() + right.size());

    int i = 0;
    int j = 0;

    while (i < std::ssize(left) && j < std::ssize(right))
    {
        if (left[i] <= right[j])
        {
            result.push_back(left[i]);
            ++i;
        }
        else
        {
            result.push_back(right[j]);
            ++j;
        }
    }

    while (i < std::ssize(left))
    {
        result.push_back(left[i]);
        ++i;
    }

    while (j < std::ssize(right))
    {
        result.push_back(right[j]); // fixed 
        ++j;
    }

    return result;
}

std::vector<int> merge_sort(std::vector<int> in)
{
    if (std::ssize(in) <= 1)
    {
        return in;
    }

    size_t mid = std::ssize(in) / 2;

    auto left = merge_sort(std::vector<int>(in.begin(), in.begin() + mid));
    auto right = merge_sort(std::vector<int>(in.begin() + mid, in.end()));

    return merge(left, right);
}

int main()
{
    std::vector<int> values = {3, 1, 4, 1, 5, 9, 2, 6};

    auto sorted = merge_sort(values);

    for (int x : sorted)
    {
        std::cout << x << " ";
    }

    std::cout << "\n";
}