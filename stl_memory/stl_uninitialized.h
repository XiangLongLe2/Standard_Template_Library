#ifndef STL_UNINITIALIZED_H
#define STL_UNINITIALIZED_H
// 等到后面用到再回来完成
template<class InputIterator,class ForwardIterator>
ForwardIterator uninitialized_copy(InputIterator first, InputIterator last, ForwardIterator result);//在已有的内存序列[result, result + (first - last)]上执行拷贝构造,复制[first,last)

template<class ForwardIterator,class T>
void unintialized_fill(ForwardIterator first, ForwardIterator last, const T& x);//在已有的内存序列[first,last)上执行拷贝构造，复制x

template<class ForwardIterator,class Size,class T>
inline ForwardIterator uninitialized_fill_n(ForwardIterator first, Size n, const T& x) {//在已有的内存序列[first,first+n)上拷贝构造，复制x
	return __uninitialized_fill_n(first, n, x, value_type(first));
}
template<class ForwardIterator, class Size, class T, class T1>
inline ForwardIterator __uninitialized_fill_n(ForwardIterator first, Size n, const T& x, T1*) {
	typedef typename __type_traits<T1>::is_POD_type is_POD;// POD: Plain Old Data
	return __uninitialized_fill_n_aux(first, n, x, is_POD());
}
template<class ForwardIterator, class Size, class T>
inline ForwardIterator uninitialized_fill_n_aux(ForwardIterator first, Size n, const T& x, __true_type) {
	return fill_n(first, n, x);
}
template<class ForwardIterator, class Size, class T>
inline ForwardIterator uninitialized_fill_n_aux(ForwardIterator first, Size n, const T& x, __false_type) {
	ForwardIterator cur = first;//未完成，没有exception handler
	for (; n > 0; --n, ++cur)
		construct(&*cur, x);
	return cur;
}

#endif //STL_UNINITIALIZED_H

