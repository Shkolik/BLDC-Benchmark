using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Windows.Markup;

namespace DataAnalizer
{
    /// <summary>
    /// EnumListExtension
    /// </summary>
    public class EnumListExtension : MarkupExtension
    {
        #region Member Variables


        private Type _EnumType;

        #endregion //Member Variables


        #region Constructor
        /// <summary>
        /// Initializes a new <see cref="EnumListExtension"/>
        /// </summary>
        public EnumListExtension()
        {
        }


        /// <summary>
        /// Initializes a new <see cref="EnumListExtension"/>
        /// </summary>
        /// <param name="enumType">The type of enum whose members are to be returned.</param>
        public EnumListExtension(Type enumType) : this(enumType, false, false)
        {
        }

        /// <summary>
        /// Initializes a new <see cref="EnumListExtension"/>
        /// </summary>
        /// <param name="enumType">The type of enum whose members are to be returned.</param>
        /// <param name="excludeEmpty">excludeEmpty</param>
        /// <param name="getFromNames">getFromNames</param>
        public EnumListExtension(Type enumType, bool excludeEmpty, bool getFromNames)
        {
            EnumType = enumType;
            ExcludeEmpty = excludeEmpty;
            GetFromNames = getFromNames;
        }


        #endregion //Constructor


        #region Properties
        /// <summary>
        /// Gets/sets the type of enumeration to return 
        /// </summary>
        public Type EnumType
        {
            get { return _EnumType; }
            set
            {
                if (value != _EnumType)
                {
                    if (null != value)
                    {
                        var enumType = Nullable.GetUnderlyingType(value) ?? value;


                        if (!enumType.IsEnum)
                            throw new ArgumentException("Type must be for an Enum.");
                    }


                    _EnumType = value;
                }
            }
        }

        /// <summary>
        /// if true - skip empty values
        /// </summary>
        public bool ExcludeEmpty { get; set; }

        /// <summary>
        /// if true - ignore description
        /// </summary>
        public bool GetFromNames { get; set; }


        #endregion //Properties


        #region Base class overrides
        /// <summary>
        /// Returns a list of items for the specified <see cref="_EnumType"/>.
        /// </summary>
        /// <param name="serviceProvider">An object that provides services for the markup extension.</param>
        /// <returns></returns>
        public override object ProvideValue(IServiceProvider serviceProvider)
        {
            if (null == _EnumType)
                throw new InvalidOperationException("The EnumType must be specified.");


            var actualEnumType = Nullable.GetUnderlyingType(_EnumType) ?? _EnumType;
            
            var items = new Dictionary<string, object>();


            // otherwise we must process the list
            foreach (Enum item in Enum.GetValues(actualEnumType))
            {

                var itemString = item.ToString();
                if (!GetFromNames)
                    itemString = EnumHelper.GetEnumDescription(item);

                if (string.IsNullOrEmpty(itemString) && ExcludeEmpty)
                    continue;

                items.Add(itemString, item);
            }

            return items;
        }

        #endregion //Base class overrides
    }

    /// <summary>
    /// EnumHelper
    /// </summary>
    public static class EnumHelper
    {
        /// <summary>
        /// GetEnumDescription
        /// </summary>
        /// <param name="enumItem">enumItem</param>
        /// <returns>Item Description</returns>
        public static string GetEnumDescription(Enum value)
        {
            if (value == null)
            {
                throw new ArgumentNullException(nameof(value));
            }

            var fieldInfo = value.GetType().GetField(value.ToString());
            var attributes = (DescriptionAttribute[])fieldInfo.GetCustomAttributes(typeof(DescriptionAttribute), false);

            return attributes.Length > 0 ? attributes[0].Description : value.ToString();
        }
    }

    public class NameObjectPair
    {
        /// <summary>
        /// Gets or sets the name.
        /// </summary>
        /// <value>The name.</value>
        public string Name
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets the value.
        /// </summary>
        /// <value>The value.</value>
        public object Value
        {
            get;
            set;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="NameValuePair"/> class.
        /// </summary>
        public NameObjectPair()
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="NameValuePair"/> class.
        /// </summary>
        /// <param name="name">The name.</param>
        /// <param name="value">The value.</param>
        public NameObjectPair(string name, object value)
        {
            Name = name;
            Value = value;
        }
    }
}
