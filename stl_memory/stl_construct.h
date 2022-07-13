#ifndef STL_CONSTRUCT_H
#define STL_CPMSTRUCT_H
#include<new.h>
template <class T1,class T2>
inline void construct(T1* p, const T2& value) {
	new (p) T1(value);
}

template<class T>
inline void destroy(T* pointer) {
	pointer->~T();
}

template<class ForwardIterator>
inline void destroy(ForwardIterator first, ForwardIterator last) {
	__destroy(first, last, value_type(first));
}

template<class ForwardIterator,class T>
inline void __destroy(ForwardIterator first, ForwardIterator last, T*) {
	typedef typename __type_traits<T>::has_trivial destructor trivial_destructor;//����type_traits�����ж��Ƿ�Ϊ�򵥶���
	__destroy_aux(first, last, trivial_destructor());
}
template<class ForwardIterator>
inline void __destroy_aux(ForwardIterator first, ForwardIterator last, __false_type) {//���Ӷ��������������
	for (; first < last; ++first)
		destroy(&*first); //Ӧ���ڵ����������н���
}
template<class ForwardIterator>
inline void __destroy_aux(ForwardIterator first, ForwardIterator last, __true_type) {}//�򵥶�������

#endif // !STL_CONSTRUCT_H
