<template>
  <v-dialog v-model="dialog" persistent width="1300">
    <v-card>
      <v-card-title>
        <span class="headline"> New Application </span>
      </v-card-title>
      <v-card-text>
        <v-form ref="form" lazy-validation>
          <v-row>
            <!--<v-col cols="12" sm="6" md="4">-->
            <v-col cols="12">
              <v-text-field
                label="*Motivation"
                :rules="[(v) => !!v || 'Motivation is required']"
                required
                v-model="newEnrollment.motivation"
                data-cy="motivationInput"
              ></v-text-field>
            </v-col>
          </v-row>
        </v-form>
      </v-card-text>
      <v-card-actions>
        <v-spacer></v-spacer>
        <v-btn
          color="blue-darken-1"
          variant="text"
          @click="$emit('close-enrollment-dialog')"
        >
          Close
        </v-btn>
        <v-btn
          v-if="canSave"
          color="blue-darken-1"
          variant="text"
          @click="createEnrollment"
          data-cy="createEnrollment"
        >
          Save
        </v-btn>
      </v-card-actions>
    </v-card>
  </v-dialog>
</template>
<script lang="ts">
import { Vue, Component, Model, Prop } from 'vue-property-decorator';
import Enrollment from '@/models/enrollment/Enrollment';
import Activity from '@/models/activity/Activity';
import RemoteServices from '@/services/RemoteServices';

@Component({})
export default class EnrollmentDialog extends Vue {
  @Model('dialog', Boolean) dialog!: boolean;
  @Prop({ type: Enrollment, required: true }) readonly enrollment!: Enrollment;
  @Prop({ type: Activity, required: true }) readonly activity!: Activity;

  newEnrollment: Enrollment = new Enrollment();
  currentActivity: Activity = new Activity();
  async created() {
    this.newEnrollment = new Enrollment(this.enrollment);
    this.newEnrollment.motivation = '';
    this.currentActivity = new Activity(this.activity);
  }
  get canSave(): boolean {
    return this.newEnrollment.motivation.trim().length >= 10;
  }

  async createEnrollment() {
    if ((this.$refs.form as Vue & { validate: () => boolean }).validate()) {
      try {
        if (this.currentActivity.id != null) {
          const result = await RemoteServices.createEnrollment(
            this.$store.getters.getUser.id,
            this.currentActivity.id,
            this.newEnrollment,
          );
          this.$emit('save-enrollment', result);
        }
      } catch (error) {
        await this.$store.dispatch('error', error);
      }
    }
  }
}
</script>

<style scoped lang="scss"></style>
