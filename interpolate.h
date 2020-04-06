
#include <LinkedList.h>

template <typename T1, typename T2>
T2 hermiteInterpolate(LinkedList<std::pair<T1, T2>> &list, T1 value,
                      double tension, double bias) {
  auto hermiteInterpolate = [](double y0, double y1, double y2, double y3,
                               double mu, double tension, double bias) {
    double m0, m1, mu2, mu3;
    double a0, a1, a2, a3;

    mu2 = mu * mu;
    mu3 = mu2 * mu;
    m0 = (y1 - y0) * (1 + bias) * (1 - tension) / 2;
    m0 += (y2 - y1) * (1 - bias) * (1 - tension) / 2;
    m1 = (y2 - y1) * (1 + bias) * (1 - tension) / 2;
    m1 += (y3 - y2) * (1 - bias) * (1 - tension) / 2;
    a0 = 2 * mu3 - 3 * mu2 + 1;
    a1 = mu3 - 2 * mu2 + mu;
    a2 = mu3 - mu2;
    a3 = -2 * mu3 + 3 * mu2;

    return (a0 * y1 + a1 * m0 + a2 * m1 + a3 * y2);
  };

  // tranverse the list and interpulate the found value
  // lower and upper bounds
  bool flag = true;
  size_t lower_idx;
  size_t upper_idx;
  for (int i = 0; i < list.size(); i++) {
    if (flag && list.get(i).first >= value) {
      flag = false;
      lower_idx = (i - 1) < 0 ? list.size() - 1 : (i - 1);
      upper_idx = i;
    }

    if (flag && i == list.size() - 1) {
      flag = false;
      lower_idx = i;
      upper_idx = 0;
    }
  }

  Serial.println(lower_idx);
  Serial.println(upper_idx);

  if (lower_idx != upper_idx) {

    // transform value from the interval
    double mu = (value - list.get(lower_idx).first) /
                (list.get(upper_idx).first -
                 list.get(lower_idx).first); // value from interval [0,1]

    return (T2)hermiteInterpolate(
        (lower_idx - 1) < 0 ? list.get(list.size() - 1).second
                            : list.get(lower_idx - 1).second,
        list.get(lower_idx).second, list.get(upper_idx).second,
        (upper_idx + 1) > list.size() - 1 ? list.get(0).second
                                          : list.get(upper_idx + 1).second,
        mu, tension, bias);
  }

  return list.get(lower_idx).second;
}
