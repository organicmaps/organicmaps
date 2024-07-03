package app.tourism.data.dto.currency;

import org.simpleframework.xml.Attribute;
import org.simpleframework.xml.Element;
import org.simpleframework.xml.Root;

@Root(name = "Valute")
public class Currency{
    @Attribute(required = false) public String ID;

    @Element(name = "CharCode") public String charCode;
    @Element(name = "Nominal") public Integer nominal;
    @Element(name = "Name") public String name;
    @Element(name = "Value") public Double value;
}

