require File.expand_path('../../../spec_helper', __FILE__)

describe "ENV.[]" do
  before :each do
    @variable_name = 'USER'
    platform_is :windows do
      @variable_name = 'USERNAME'
    end
  end

  it "returns the specified environment variable" do
    ENV[@variable_name].should == username
  end

  it "returns nil if the variable isn't found" do
    ENV["this_var_is_never_set"].should == nil
  end
  
  it "returns only frozen values" do
    ENV[@variable_name].frozen?.should == true
    ENV["returns_only_frozen_values"] = "a non-frozen string"
    ENV["returns_only_frozen_values"].frozen?.should == true
  end

  ruby_version_is "1.9" do
    it "uses the locale encoding" do
      ENV[@variable_name].encoding.should == Encoding.find('locale')
      ENV["uses_the_locale_encoding"] = "a binary string".force_encoding('binary')
      ENV["uses_the_locale_encoding"].encoding.should == Encoding.find('locale')
    end
  end
end
