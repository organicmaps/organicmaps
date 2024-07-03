package app.tourism.data.dto.currency;

import org.simpleframework.xml.Attribute;
import org.simpleframework.xml.ElementList;
import org.simpleframework.xml.Root;

import java.util.List;

import app.tourism.domain.models.profile.CurrencyRates;

@Root(name = "ValCurs")
public class CurrenciesList {
    @Attribute(required = false, name = "Date") public String date;
    @Attribute(required = false) public String name;

    @ElementList(name = "Valute", inline = true)
    public List<Currency> currencies;
}

