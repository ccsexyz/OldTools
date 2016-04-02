#ifndef BIGINT_BIGINT_H
#define BIGINT_BIGINT_H

#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>
#include <string>
#include <vector>

class BigInt final {
public:
    BigInt(int64_t that = 0) {
        sign_ = that >= 0;
        if (!sign_)
            that = -that;
        if (that == 0) {
            nums_.push_back(0);
            return;
        }
        while (that) {
            nums_.push_back(that % radix_);
            that /= radix_;
        }
    }
    BigInt(int32_t that) : BigInt(static_cast<int64_t>(that)) {}
    BigInt(uint64_t that) {
        sign_ = true;
        if (that > radix_) {
            nums_.push_back(that / radix_);
            nums_.push_back(that % radix_);
        } else {
            nums_.push_back(that);
        }
    }
    BigInt(uint32_t that) : BigInt(static_cast<uint64_t>(that)) {}

    BigInt(const std::string &that, int system = 10)
        : BigInt(that.data(), system) {}
    BigInt(const char *that, int system = 10) {
        int i = 0;
        int len = ::strlen(that);
        nums_.push_back(0);
        if (len == 0 || (len == 1 && (that[0] == '-' || that[0] == '+'))) {
            return;
        }

        sign_ = true;
        if (that[0] == '-') {
            sign_ = false;
            i++;
        } else if (that[0] == '+') {
            i++;
        }

        while (i < len) {
            Mul(system);
            int k = 0;
            auto c = that[i];
            if (c >= '0' && c <= '9') {
                k = c - '0';
            } else if (c >= 'A' && c <= 'F') {
                k = c - 'A' + 10;
            } else if (c >= 'a' && c <= 'f') {
                k = c - 'a' + 10;
            } else {
                k = 0;
            }
            Add(k);
            i++;
        }
    }

    BigInt(const BigInt &that)
        : sign_(that.sign_), nums_(that.nums_.begin(), that.nums_.end()) {}
    BigInt(BigInt &&that)
        : sign_(that.sign_), nums_(that.nums_.begin(), that.nums_.end()) {}

    BigInt &operator=(const BigInt &that) {
        nums_ = that.nums_;
        sign_ = that.sign_;
        return *this;
    }

    std::string toString(uint32_t system = 10) const {
        static const char *t = "0123456789ABCDEF";
        std::string ret;
        if (isZero()) {
            ret = "0";
            return ret;
        } else if (isNegative()) {
            ret += "-";
        }
        BigInt sys = system;
        BigInt value = *this;
        while (!value.isZero()) {
            BigInt rest = value;
            rest %= sys;
            value /= sys;
            ret += t[rest.toInt()];
        }
        return std::string(ret.rbegin(), ret.rend());
    }

    void toString(std::string &str, uint32_t system = 10) const {
        static const char *t = "0123456789ABCDEF";
        str = toString(system);
    }

    void Add(const BigInt &that) {
        uint32_t carry = 0;
        if (nums_.size() < that.nums_.size())
            nums_.resize(that.nums_.size());
        for (int i = 0; i < that.nums_.size(); i++) {
            uint64_t temp = nums_[i];
            temp += that.nums_[i] + carry;
            if (temp >= radix_) {
                carry = temp / radix_;
                temp %= radix_;
            } else {
                carry = 0;
            }
            nums_[i] = temp;
        }
        if (carry != 0) {
            if (nums_.size() == that.nums_.size()) {
                nums_.push_back(carry);
            } else {
                int i = that.nums_.size();
                while (carry != 0) {
                    uint64_t temp = nums_[i];
                    temp += carry;
                    carry = temp / radix_;
                    if (carry != 0) {
                        temp %= radix_;
                    }
                    nums_[i] = temp;
                }
            }
        }
    }
    void Sub(const BigInt &that) {
        uint32_t n = 0;
        uint32_t borrow = 0;
        for (int i = 0; i < that.nums_.size(); i++) {
            nums_[i] -= borrow;
            if (nums_[i] < that.nums_[i]) {
                borrow = 1;
                nums_[i] += radix_;
            } else {
                borrow = 0;
            }
            nums_[i] -= that.nums_[i];
        }
        for (int i = that.nums_.size(); borrow != 0 && i < nums_.size(); i++) {
            if (nums_[i] < borrow) {
                borrow = 1;
                nums_[i] += radix_ - borrow;
            } else {
                nums_[i] -= borrow;
                borrow = 0;
            }
        }
        for (int i = 0; i < nums_.size(); i++) {
            if (nums_[i]) {
                n = i + 1;
            }
        }
        nums_.resize(n == 0 ? 1 : n);
    }
    void Div(const BigInt &that, BigInt &rest) {
        rest = BigInt();
        while (!CompareNoSign(that)) {
            uint64_t div = nums_.back();
            uint64_t num = that.nums_.back();
            uint64_t len = nums_.size() - that.nums_.size();
            if (div == num && len == 0) {
                rest.Add(BigInt(1));
                Sub(that);
                break;
            }
            if (div <= num && len > 0) {
                len--;
                div = div * radix_ + nums_[nums_.size() - 2];
            }
            div /= num + 1;
            BigInt multi = div;
            if (len > 0) {
                multi.shiftLeft(len);
            }
            BigInt tmp = that;
            rest.Add(multi);
            tmp.Mul(multi);
            Sub(tmp);
        }
        Swap(rest);
    }
    void Mul(const BigInt &that) {
        BigInt thisCopy(*this);
        nums_ = std::vector<uint32_t>(nums_.size() + that.nums_.size(), 0);
        uint32_t carry = 0;
        for (int i = 0; i < that.nums_.size(); i++) {
            for (int j = 0; j < thisCopy.nums_.size(); j++) {
                uint64_t temp = that.nums_[i];
                temp *= thisCopy.nums_[j];
                temp += carry;
                temp += nums_[i + j];
                nums_[i + j] = temp % radix_;
                carry = temp / radix_;
            }
            if (carry != 0) {
                nums_[i + thisCopy.nums_.size()] += carry;
                carry = 0;
            }
        }
        if (nums_.back() == 0)
            nums_.pop_back();
    }
    void Power(const BigInt &n) {
        if (n.isZero()) {
            *this = BigInt(1);
            return;
        }

        BigInt t(1);
        Swap(t);

        for (uint64_t i = 0; i < n.GetTotalBits(); i++) {
            if (n.TestBit(i)) {
                Mul(t);
            }
            BigInt tmp(t);
            t.Mul(tmp);
        }
    }

    void Swap(BigInt &that) {
        std::swap(nums_, that.nums_);
        std::swap(sign_, that.sign_);
    }

    // return if *this < that
    bool Compare(const BigInt &that) const {
        if (sign_ ^ that.sign_) {
            return !sign_;
        }
        return sign_ == CompareNoSign(that);
    }
    bool CompareNoSign(const BigInt &that) const {
        bool ret = false;
        if (nums_.size() < that.nums_.size()) {
            ret = true;
        } else if (nums_.size() == that.nums_.size()) {
            for (int i = nums_.size() - 1; i > -1; --i) {
                if (nums_[i] != that.nums_[i]) {
                    ret = nums_[i] < that.nums_[i];
                    break;
                }
            }
        }
        return ret;
    }
    bool Equals(const BigInt &that) const {
        return sign_ == that.sign_ && nums_.size() == that.nums_.size() &&
               nums_ == that.nums_;
    }
    bool isOdd() const { return nums_[0] % 2 == 1; }
    bool isEven() const { return nums_[0] % 2 == 0; }

    void shiftLeft(uint32_t i) {
        if (i == 0)
            return;
        std::vector<uint32_t> tmp(i, 0);
        tmp.insert(tmp.end(), nums_.begin(), nums_.end());
        nums_ = tmp;
    }

    void shiftRight(uint32_t i) {
        if (i == 0)
            return;
        if (i > nums_.size() - 1) {
            nums_ = std::vector<uint32_t>(0);
            return;
        }
        for (int j = 0; j < nums_.size() - i; j++) {
            nums_[j] = nums_[j + i];
        }
        nums_.resize(nums_.size() - i);
    }

    BigInt &operator+=(const BigInt &that) {
        if (!(sign_ ^ that.sign_)) {
            Add(that);
        } else {
            if (CompareNoSign(that)) {
                BigInt tmp = that;
                tmp.Sub(*this);
                *this = tmp;
            } else {
                Sub(that);
            }
        }
        if (isZero())
            sign_ = true;
        return *this;
    }
    BigInt &operator-=(const BigInt &that) {
        if (sign_ ^ that.sign_) {
            Add(that);
        } else {
            if (CompareNoSign(that)) {
                sign_ = !sign_;
                BigInt tmp = that;
                tmp.Sub(*this);
                nums_ = tmp.nums_;
            } else {
                Sub(that);
            }
        }
        if (isZero())
            sign_ = true;
        return *this;
    }
    BigInt &operator*=(const BigInt &that) {
        Mul(that);
        if (isZero()) {
            sign_ = true;
        } else {
            sign_ = !(sign_ ^ that.sign_);
        }
        return *this;
    }
    BigInt &operator/=(const BigInt &that) {
        BigInt rest;
        bool savedSign = sign_;
        Div(that, rest);
        if (isZero()) {
            sign_ = true;
        } else {
            sign_ = !(savedSign ^ that.sign_);
        }
        return *this;
    }
    BigInt &operator%=(const BigInt &that) {
        BigInt rest;
        bool savedSign = sign_;
        Div(that, rest);
        *this = rest;
        sign_ = savedSign || that.sign_;
        return *this;
    }
    BigInt &operator^=(const BigInt &that) {
        bool savedSign = sign_;
        Power(that);
        if (that.isOdd()) {
            sign_ = savedSign;
        } else {
            sign_ = true;
        }
        return *this;
    }

    bool isPositive() const {
        return sign_ && !(nums_.size() == 1 && nums_[0] == 0);
    }
    bool isZero() const { return nums_.size() == 1 && nums_[0] == 0; }
    bool isNegative() const { return !sign_; }

    uint64_t GetTotalBits() const {
        uint64_t ret = 32 * (nums_.size() - 1);
        uint64_t backNum = nums_.back();
        while (backNum) {
            backNum >>= 1;
            ret++;
        }
        return ret;
    }

    bool TestBit(uint64_t bits) const {
        uint64_t wp = bits / 32;
        uint64_t bp = bits % 32;

        if (wp >= nums_.size())
            return false;

        uint64_t mask = 1 << bp;
        return (nums_[wp] & mask) != 0;
    }
    int toInt() const { return nums_.front(); }
    std::vector<uint32_t> getNums() const { return nums_; }

private:
    const uint64_t radix_ = 1UL << 32;
    uint32_t parse(const char *start, const char *end, int system = 10) {
        uint32_t ret = 0;
        while (start != end) {
            ret *= system;
            ret += *start++;
        }
        return ret;
    }

private:
    bool sign_; // if sign_ == true, the integer is greater than zero includes
                // zero
    std::vector<uint32_t> nums_;
};

inline BigInt operator+(const BigInt &lhs, const BigInt &rhs) {
    BigInt ret = lhs;
    ret += rhs;
    return ret;
}
inline BigInt operator-(const BigInt &lhs, const BigInt &rhs) {
    BigInt ret = lhs;
    ret -= rhs;
    return ret;
}
inline BigInt operator*(const BigInt &lhs, const BigInt &rhs) {
    BigInt ret = lhs;
    ret *= rhs;
    return ret;
}
inline BigInt operator/(const BigInt &lhs, const BigInt &rhs) {
    BigInt ret = lhs;
    ret /= rhs;
    return ret;
}
inline BigInt operator%(const BigInt &lhs, const BigInt &rhs) {
    BigInt ret = lhs;
    ret %= rhs;
    return ret;
}
inline BigInt operator^(const BigInt &lhs, const BigInt &rhs) {
    BigInt ret = lhs;
    ret ^= rhs;
    return ret;
}
inline bool operator<(const BigInt &lhs, const BigInt &rhs) {
    return lhs.Compare(rhs);
}
inline bool operator>(const BigInt &lhs, const BigInt &rhs) {
    return rhs.Compare(lhs);
}
inline bool operator==(const BigInt &lhs, const BigInt &rhs) {
    return lhs.Equals(rhs);
}
inline bool operator<=(const BigInt &lhs, const BigInt &rhs) {
    return !rhs.Compare(lhs);
}
inline bool operator>=(const BigInt &lhs, const BigInt &rhs) {
    return !lhs.Compare(rhs);
}
static BigInt ModPow(const BigInt &A, const BigInt &B, const BigInt &N) {
    BigInt k = 1;
    BigInt n = A % N;

    for (uint64_t i = 0; i < B.GetTotalBits(); i++) {
        if (B.TestBit(i)) {
            k = (k * n) % N;
        }
        n = (n * n) % N;
    }

    return k;
}

#endif // BIGINT_BIGINT_H
