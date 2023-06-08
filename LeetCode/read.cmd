
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

#include <stdio.h>
#include <string.h>

char* removeKDigits(char* num, int k) {
    int len = strlen(num);
    int top = 0;  // 栈顶指针

    // 遍历每一位数字
    for (int i = 0; i < len; i++) {
        while (top > 0 && k > 0 && num[i] < num[top - 1]) {
            // 如果当前数字比栈顶数字小，则弹出栈顶数字
            top--;
            k--;
        }
        if (top > 0 || num[i] != '0') {
            // 将当前数字入栈
            num[top++] = num[i];
        }
    }

    // 如果还有剩余的删除次数，继续从栈中删除数字
    while (top > 0 && k > 0) {
        top--;
        k--;
    }

    // 从栈中构建结果字符串
    num[top] = '\0';

    // 将栈中的数字反转得到最终结果
    for (int i = 0, j = top - 1; i < j; i++, j--) {
        char temp = num[i];
        num[i] = num[j];
        num[j] = temp;
    }

    // 处理结果为空的情况，返回"0"
    if (top == 0) {
        num[0] = '0';
        num[1] = '\0';
    }

    return num;
}

int main() {
    char num[] = "1432219";
    int k = 3;

    char* smallestNum = removeKDigits(num, k);
    printf("Smallest number after removing %d digits: %s\n", k, smallestNum);

    return 0;
}



