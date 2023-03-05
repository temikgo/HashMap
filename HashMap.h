#ifndef HASHMAP_HASHMAP_H
#define HASHMAP_HASHMAP_H

#include <iostream>
#include <initializer_list>
#include <list>
#include <memory>
#include <stdexcept>
#include <vector>

template<class KeyType, class ValueType, class Hash = std::hash<KeyType> > class HashMap {
    public:
        using Pair = std::pair<const KeyType, ValueType>;

        class iterator {
            public:
                iterator() = default;

                iterator(typename std::list<std::shared_ptr<Pair>>::iterator it) : it_(it) {}

                iterator& operator++() {
                    ++it_;
                    return *this;
                }

                iterator operator++(int) {
                    iterator tmp = *this;
                    ++it_;
                    return tmp;
                }

                Pair& operator*() {
                    return **it_;
                }

                Pair* operator->() {
                    return &**it_;
                }

                bool operator==(const iterator& other) {
                    return other.it_ == it_;
                }

                bool operator!=(const iterator& other) {
                    return !(*this == other);
                }

                typename std::list<std::shared_ptr<Pair>>::iterator it() const {
                    return it_;
                }

            private:
                typename std::list<std::shared_ptr<Pair>>::iterator it_;
        };
        class const_iterator {
        public:
            const_iterator() = default;

            const_iterator(iterator it) : it_(it.it()) {}
            const_iterator(typename std::list<std::shared_ptr<Pair>>::const_iterator it) : it_(it) {}

            const_iterator& operator++() {
                ++it_;
                return *this;
            }

            const_iterator operator++(int) {
                const_iterator tmp = *this;
                ++it_;
                return tmp;
            }

            const Pair& operator*() {
                return **it_;
            }

            const Pair* operator->() {
                return &**it_;
            }

            bool operator==(const const_iterator& other) {
                return other.it_ == it_;
            }

            bool operator!=(const const_iterator& other) {
                return !(*this == other);
            }

            typename std::list<std::shared_ptr<Pair>>::const_iterator it() const {
                return it_;
            }

        private:
            typename std::list<std::shared_ptr<Pair>>::const_iterator it_;
        };

        HashMap<int, int>& operator=(const HashMap<int, int>& hash_map) {
            hasher_ = hash_map.hasher_;
            table_ = hash_map.table_;
            state_ = hash_map.state_;
            occupancy_ = hash_map.occupancy_;
            capacity_ = hash_map.capacity_;
            size_ind_ = hash_map.size_ind_;
            poses_ = hash_map.poses_;
            values_ = hash_map.values_;
            poses_iterators_ = hash_map.poses_iterators_;
            values_iterators_ = hash_map.values_iterators_;
            return *this;
        }

        HashMap(Hash hasher=Hash()) : hasher_(hasher) {
            occupancy_ = 0;
            capacity_ = 0;
            size_ind_ = 0;
            table_.resize(sizes_[size_ind_]);
            state_.resize(sizes_[size_ind_], 0);
            poses_iterators_.resize(sizes_[size_ind_]);
            values_iterators_.resize(sizes_[size_ind_]);
        }
        template<class Iterator>
        HashMap(Iterator begin, Iterator end, Hash hasher=Hash()) : HashMap(hasher) {
            while (begin != end) {
                insert(*begin);
                ++begin;
            }
        }
        HashMap(std::initializer_list<Pair> list, Hash hasher=Hash())
            : HashMap(list.begin(), list.end(), hasher) {
        }

        size_t size() const {
            return occupancy_;
        }
        bool empty() const {
            return occupancy_ == 0;
        }

        Hash hash_function() const {
            return hasher_;
        }

        void insert(Pair elem) {
            KeyType key = elem.first;
            size_t k = hasher_(key);
            size_t h = Hash1(k);
            while (state_[h] > 0 && (!(table_[h]->first == key) || state_[h] == 2)) {
                h = (h + Hash2(k)) % sizes_[size_ind_];
            }
            if (state_[h] == 1 && table_[h]->first == key) {
                return;
            }
            ++occupancy_;
            if (!state_[h]) {
                ++capacity_;
            }
            state_[h] = 1;
            table_[h] = std::make_shared<Pair>(elem);
            poses_.push_front(h);
            values_.push_front(table_[h]);
            poses_iterators_[h] = poses_.begin();
            values_iterators_[h] = values_.begin();
            if (static_cast<double>(capacity_) / static_cast<double>(sizes_[size_ind_]) > ratio_) {
                rebuild();
            }
        }
        void erase(KeyType key) {
            size_t k = hasher_(key);
            size_t h = Hash1(k);
            while (state_[h] > 0 && (!(table_[h]->first == key) || state_[h] == 2)) {
                h = (h + Hash2(k)) % sizes_[size_ind_];
            }
            if (state_[h] == 1 && table_[h]->first == key) {
                --occupancy_;
                state_[h] = 2;
                poses_.erase(poses_iterators_[h]);
                values_.erase(values_iterators_[h].it());
            }
        }

        iterator begin() {
            return values_.begin();
        }
        iterator end() {
            return values_.end();
        }
        const_iterator begin() const {
            return values_.begin();
        }
        const_iterator end() const {
            return values_.end();
        }

        iterator find(KeyType key) {
            size_t k = hasher_(key);
            size_t h = Hash1(k);
            while (state_[h] > 0 && (!(table_[h]->first == key) || state_[h] == 2)) {
                h = (h + Hash2(k)) % sizes_[size_ind_];
            }
            if (state_[h] == 1 && table_[h]->first == key) {
                return values_iterators_[h];
            }
            return values_.end();
        }
        const_iterator find(KeyType key) const {
            size_t k = hasher_(key);
            size_t h = Hash1(k);
            while (state_[h] > 0 && (!(table_[h]->first == key) || state_[h] == 2)) {
                h = (h + Hash2(k)) % sizes_[size_ind_];
            }
            if (state_[h] == 1 && table_[h]->first == key) {
                return values_iterators_[h];
            }
            return values_.end();
        }

        ValueType& operator [](KeyType key) {
            size_t k = hasher_(key);
            size_t h = Hash1(k);
            while (state_[h] > 0 && (!(table_[h]->first == key) || state_[h] == 2)) {
                h = (h + Hash2(k)) % sizes_[size_ind_];
            }
            if (state_[h] == 1 && table_[h]->first == key) {
                return table_[h]->second;
            }
            ++occupancy_;
            if (!state_[h]) {
                ++capacity_;
            }
            Pair elem = {key, ValueType()};
            state_[h] = 1;
            table_[h] = std::make_shared<Pair>(elem);
            poses_.push_front(h);
            values_.push_front(table_[h]);
            poses_iterators_[h] = poses_.begin();
            values_iterators_[h] = values_.begin();
            if (static_cast<double>(capacity_) / static_cast<double>(sizes_[size_ind_]) > ratio_) {
                rebuild();
                k = hasher_(key);
                h = Hash1(k);
                while (state_[h] > 0 && (!(table_[h]->first == key) || state_[h] == 2)) {
                    h = (h + Hash2(k)) % sizes_[size_ind_];
                }
                return table_[h]->second;
            }
            return table_[h]->second;
        }
        const ValueType& at(KeyType key) const {
            size_t k = hasher_(key);
            size_t h = Hash1(k);
            while (state_[h] > 0 && (!(table_[h]->first == key) || state_[h] == 2)) {
                h = (h + Hash2(k)) % sizes_[size_ind_];
            }
            if (state_[h] == 1 && table_[h]->first == key) {
                return table_[h]->second;
            }
            throw std::out_of_range("");
        }

        void clear() {
            for (size_t pos: poses_) {
                state_[pos] = 0;
                --occupancy_;
                --capacity_;
            }
            poses_.clear();
            values_.clear();
            poses_iterators_.clear();
            values_iterators_.clear();
        }

        void print() {
            for (auto value: values_) {
                std::cout << value->first << " " << value->second << "\n";
            }
        }

    private:
        const double ratio_ = 0.5;
        const std::vector<size_t> sizes_ =
                {3, 7, 17, 37, 79, 163, 331, 673, 1361, 2729, 5471, 10949,
                 21911, 43853, 87719, 175447, 350899, 701819, 1403641};

        Hash hasher_;
        std::vector<std::shared_ptr<Pair>> table_;
        std::vector<size_t> state_;
        size_t occupancy_, capacity_, size_ind_;
        std::list<size_t> poses_;
        std::list<std::shared_ptr<Pair>> values_;
        std::vector<std::list<size_t>::iterator> poses_iterators_;
        std::vector<iterator> values_iterators_;

        size_t Hash1(size_t k) const {
            return k % sizes_[size_ind_];
        }
        size_t Hash2(size_t k) const {
            return 1 + k % (sizes_[size_ind_] - 1);
        }

        void rebuild() {
            ++size_ind_;
            std::vector<std::shared_ptr<Pair>> new_table(sizes_[size_ind_]);
            state_.assign(sizes_[size_ind_], 0);
            poses_iterators_.resize(sizes_[size_ind_]);
            values_iterators_.resize(sizes_[size_ind_]);
            std::vector<size_t> hashes;
            values_.clear();
            for (size_t pos: poses_) {
                Pair elem = *table_[pos];
                KeyType key = elem.first;
                size_t k = hasher_(key);
                size_t h = Hash1(k);
                while (state_[h] > 0) {
                    h = (h + Hash2(k)) % sizes_[size_ind_];
                }
                state_[h] = 1;
                new_table[h] = std::make_shared<Pair>(elem);
                hashes.push_back(h);
            }
            table_ = new_table;
            poses_.clear();
            for (size_t h : hashes) {
                poses_.push_front(h);
                values_.push_front(table_[h]);
                poses_iterators_[h] = poses_.begin();
                values_iterators_[h] = values_.begin();
            }
        }
};

#endif //HASHMAP_HASHMAP_H
