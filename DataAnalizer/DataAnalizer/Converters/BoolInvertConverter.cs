using System;
using System.Globalization;
using System.Windows.Data;

namespace DataAnalizer.Converters
{
    /// <summary>
    /// Use to invert boolean value
    /// </summary>
    public sealed class BoolInvertConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            return !(bool)value;
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotSupportedException();
        }
    }
}
