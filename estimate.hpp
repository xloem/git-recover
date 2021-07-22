// thinking of making estimate
// to produce estimate, usually we want an implementation and some data the implementation needs
// we could be the implementation
// we could also use templates somehow
// we could also pass individual functions
//
// also want estimate to be easily replacable with simple clear values.
// // that's like a use of estimate implementation where the value is its constructor information

// uhhhh pass value to estimate when calculating

template<typename T>
class estimate_impl
{
public:
	virtual operator T() const = 0;
	virtual T min() const = 0;
	virtual T max() const = 0;
};

template<typename T>
class estimate : public estimate_impl<T>
{
public:
	estimate(estimate_impl<T> const & impl);
	estimate(T const & value);
	estimate(estimate const &) = default;

	virtual operator T() const override final;
	virtual T min() const override final;
	virtual T max() const override final;

private:
	estimate_impl<T> * impl;
	T value;
};

template <typename T>
estimate<T>::estimate(estimate_impl<T> const & impl)
: impl(&impl)
{ }

template <typename T>
estimate<T>::estimate(T const & value)
: impl(nullptr),
  value(value)
{ }

template <typename T>
estimate<T>::operator T() const
{
	return impl ? impl->operator T() : value;
}

template <typename T>
estimate<T>::min() const
{
	return impl ? impl->min() : value;
}

template <typename T>
estimate<T>::max() const
{
	return impl ? impl->max() : value;
}
