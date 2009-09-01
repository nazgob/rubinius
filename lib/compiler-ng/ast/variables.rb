module Rubinius
  module AST
    class BackRef < Node
      attr_accessor :kind

      def self.from(p, ref)
        node = BackRef.new p.compiler
        node.kind = ref
        node
      end

      def bytecode(g)
        g.push_variables
        g.push_literal @kind
        g.send :back_ref, 1
      end
    end

    class NthRef < Node
      attr_accessor :which

      def self.from(p, ref)
        node = NthRef.new p.compiler
        node.which = ref
        node
      end

      def bytecode(g)
        pos(g)

        g.push_variables
        g.push @which
        g.send :nth_ref, 1
      end
    end

    class CVar < Node
      attr_accessor :name

      def self.from(p, name)
        node = CVar.new p.compiler
        node.name = name
        node
      end

      def in_module
        @in_module = true
      end

      def or_bytecode(g)
        if @in_module
          g.push :self
        else
          g.push_scope
        end

        done =     g.new_label
        notfound = g.new_label

        g.push_literal @name
        g.send :class_variable_defined?, 1
        g.gif notfound

        # Ok, we the value exists, get it.
        bytecode(g)
        g.dup
        g.git done
        g.pop

        # yield to generate the code for when it's not found
        notfound.set!
        yield

        done.set!
      end

      def bytecode(g)
        if @in_module
          g.push :self
        else
          g.push_scope
        end
        g.push_literal @name
        g.send :class_variable_get, 1
      end
    end

    class CVarAssign < Node
      attr_accessor :name, :value

      def self.from(p, name, value)
        node = CVarAssign.new p.compiler
        node.name = name
        node.value = value
        node
      end

      def in_module
        @in_module = true
      end

      def children
        [@value]
      end

      def bytecode(g)
        if @in_module
          g.push :self
        else
          g.push_scope
        end

        if @value
          g.push_literal @name
          @value.bytecode(g)
        else
          g.swap
          g.push_literal @name
          g.swap
        end

        g.send :class_variable_set, 2
      end
    end

    class CVarDeclare < CVarAssign
      def self.from(p, name, value)
        node = CVarDeclare.new p.compiler
        node.name = name
        node.value = value
        node
      end
    end

    class GVar < Node
      attr_accessor :name

      def self.from(p, name)
        node = GVar.new p.compiler
        node.name = name
        node
      end

      def bytecode(g)
        pos(g)

        if @name == :$!
          g.push_exception
        elsif @name == :$~
          g.push_variables
          g.send :last_match, 0
        else
          g.push_const :Rubinius
          g.find_const :Globals
          g.push_literal @name
          g.send :[], 1
        end
      end
    end

    class GVarAssign < Node
      attr_accessor :name, :value

      def self.from(p, name, expr)
        node = GVarAssign.new p.compiler
        node.name = name
        node.value = expr
        node
      end

      def bytecode(g)
        pos(g)

        # @value can to be present if this is coming via an masgn, which means
        # the value is already on the stack.
        if @name == :$!
          @value.bytecode(g) if @value
          g.raise_exc
        elsif @name == :$~
          if @value
            g.find_cpath_top_const :Regexp
            @value.bytecode(g)
            g.send :last_match=, 1
          else
            g.find_cpath_top_const :Regexp
            g.swap
            g.send :last_match=, 1
          end
        else
          if @value
            g.push_const :Rubinius
            g.find_const :Globals
            g.push_literal @name
            @value.bytecode(g)
            g.send :[]=, 2
          else
            g.push_const :Rubinius
            g.find_const :Globals
            g.swap
            g.push_literal @name
            g.swap
            g.send :[]=, 2
          end
        end
      end
    end

    class SplatAssignment < Node
      attr_accessor :name, :value

      def self.from(p, value)
        node = SplatAssignment.new p.compiler
        node.value = value
        node
      end

      def children
        [@value]
      end

      def bytecode(g)
        g.cast_array
        @value.bytecode(g)
      end
    end

    class EmptySplat < Node
      def self.from(p)
        EmptySplat.new p.compiler
      end
    end

    class IVar < Node
      attr_accessor :name

      def self.from(p, name)
        node = IVar.new p.compiler
        node.name = name
        node
      end

      def bytecode(g)
        pos(g)

        g.push_ivar @name
      end
    end

    class IVarAssign < Node
      attr_accessor :name, :value

      def self.from(p, name, value)
        node = IVarAssign.new p.compiler
        node.name = name
        node.value = value
        node
      end

      def bytecode(g)
        pos(g)

        @value.bytecode(g) if @value
        g.set_ivar @name
      end
    end

    class LocalAccess < LocalVariable
      attr_accessor :variable

      def self.from(p, name)
        node = LocalAccess.new p.compiler
        node.name = name
        node
      end

      def bytecode(g)
        @variable.get_bytecode(g)
      end
    end

    class LocalAssignment < LocalVariable
      attr_accessor :value, :depth, :variable

      def self.from(p, name, value)
        node = LocalAssignment.new p.compiler
        node.name = name
        node.value = value
        node
      end

      def children
        [@value]
      end

      def bytecode(g)
        pos(g)

        if @value
          @value.bytecode(g)
        end

        @variable.set_bytecode(g)
      end
    end

    class MAsgn < Node
      attr_accessor :left, :right, :splat

      def self.from(p, left, right, splat)
        node = MAsgn.new p.compiler
        node.left = left
        node.right = right

        if splat.kind_of? Node
          node.splat = SplatAssignment.from p, splat
        elsif splat
          node.splat = EmptySplat.from p
        end

        node.fixed if right.kind_of? ArrayLiteral

        node
      end

      def fixed
        @fixed = true
      end

      def children
        [@right, @left, @splat]
      end

      def pad_short(g)
        short = @left.body.size - @right.body.size
        short.times { g.push :nil } if short > 0
      end

      def pop_excess(g)
        excess = @right.body.size - @left.body.size
        excess.times { g.pop } if excess > 0
      end

      def make_array(g)
        size = @right.body.size - @left.body.size
        g.make_array size if size > 0
      end

      def rotate(g)
        if @splat
          size = @left.body.size + 1
        else
          size = @right.body.size
        end
        g.rotate size
      end

      def in_block
        @in_block = true
      end

      def bytecode(g)
        if @fixed
          pad_short(g) if @left unless @splat
          @right.body.each { |x| x.bytecode(g) }

          if @left
            make_array(g) if @splat
            rotate(g)

            @left.body.each do |x|
              x.bytecode(g)
              g.pop
            end

            pop_excess(g) unless @splat
          end
        else
          if @right
            @right.bytecode(g)
            g.cast_array
          end

          if @left
            @left.body.each do |x|
              g.shift_array
              g.cast_array if x.kind_of? MAsgn
              x.bytecode(g)
              g.pop
            end
          end
        end

        @splat.bytecode(g) if @splat

        unless @in_block
          g.pop unless @fixed
          g.push :true
        end
      end
    end
  end
end
