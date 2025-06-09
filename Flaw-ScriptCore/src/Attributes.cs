using System;

namespace Flaw
{
    [AttributeUsage(AttributeTargets.Property | AttributeTargets.Field)]
    public class ExposeAttribute : Attribute
    {
    }
}