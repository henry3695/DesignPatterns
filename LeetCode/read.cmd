
给你一个以字符串表示的非负整数 num 和一个整数 k ，
移除这个数中的 k 位数字，使得剩下的数字最小。
请你以字符串形式返回这个最小的数字。


#include <iostream>
#include <string>
#include <stack>

std::string removeKdigits(const std::string& num, int k) {
    std::stack<char> st;
    std::string result = "";

    // 遍历每个数字
    for (char digit : num) {
        // 移除比当前数字大的栈顶元素，直到栈为空或者已经移除了k位数字
        while (!st.empty() && k > 0 && st.top() > digit) {
            st.pop();
            k--;
        }
        // 将当前数字入栈
        st.push(digit);
    }

    // 如果仍需要移除数字，则从栈顶开始移除
    while (k > 0 && !st.empty()) {
        st.pop();
        k--;
    }

    // 构建最小数字的字符串
    while (!st.empty()) {
        result = st.top() + result;
        st.pop();
    }

    // 去除前导零
    while (result.length() > 1 && result[0] == '0') {
        result = result.substr(1);
    }

    return result;
}

int main() {
    std::string num = "1432219";
    int k = 3;
    std::string minNum = removeKdigits(num, k);
    std::cout << "The smallest number after removing " << k << " digits: " << minNum << std::endl;

    return 0;
}
