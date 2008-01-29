require 'mspec/expectations'
require 'mspec/runner/actions/timer'
require 'mspec/runner/actions/tally'

class DottedFormatter
  attr_reader :timer, :tally
  
  def initialize
    @states = []
  end
  
  def register
    @timer = TimerAction.new
    @timer.register
    @tally = TallyAction.new
    @tally.register
    
    MSpec.register :start, self
    MSpec.register :after, self
    MSpec.register :finish, self
  end
  
  def after(state)
    unless state.exception?
      print "."
    else
      @states << state
      print failure?(state) ? "F" : "E"
    end
  end
  
  def finish
    print "\n"
    count = 0
    @states.each do |state|
      state.exceptions.each do |exc|
        outcome = failure?(state) ? "FAILED" : "ERROR"
        print "\n#{count += 1})\n#{state.description} #{outcome}\n"
        print (exc.message.empty? ? "<No message>" : exc.message) + ": \n"
        print backtrace(exc)
        print "\n"
      end
    end
    print "\n#{@timer.format}\n\n#{@tally.format}\n"
  end
  
  def print(str)
    $stdout.print str
  end
  private :print
  
  def failure?(state)
    state.exceptions.all? { |e| state.failure?(e) }
  end
  private :failure?
  
  def backtrace(exc)
    begin
      return exc.awesome_backtrace.show
    rescue Exception
      return exc.backtrace && exc.backtrace.join("\n")
    end
  end
  private :backtrace
end
