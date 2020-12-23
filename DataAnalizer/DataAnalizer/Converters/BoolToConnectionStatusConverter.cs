using System;
using System.Globalization;
using System.Windows.Data;

namespace DataAnalizer.Converters
{
    /// <summary>
    /// Use to convert boolean value to connection action like Connect or Disconnect
    /// </summary>
    public sealed class BoolToConnectionActionConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            return (bool)value ? "Disconnect" : "Connect";
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotSupportedException();
        }
    }
}
